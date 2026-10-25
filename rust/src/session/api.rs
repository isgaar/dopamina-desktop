// Location: rust/src/session/api.rs
// Function: Asynchronous Dopamine Core authentication client for login, QR
// handoff consumption, session validation, connectivity, and logout.
// All Rights Reserved isgaar

use std::fs;
use std::time::Duration;

use reqwest::Client;
use serde_json::{json, Value};

use super::model::SessionUser;

const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);
const REQUEST_TIMEOUT: Duration = Duration::from_secs(10);

#[derive(Debug, Clone)]
pub struct AuthOutcome {
    pub status: u16,
    pub user: Option<SessionUser>,
    pub error: Option<String>,
}

impl AuthOutcome {
    pub fn ok(&self) -> bool {
        (200..300).contains(&self.status) && self.user.is_some()
    }

    fn network(error: reqwest::Error) -> Self {
        Self {
            status: 0,
            user: None,
            error: Some(error.to_string()),
        }
    }
}

fn http() -> Client {
    Client::builder()
        .connect_timeout(CONNECT_TIMEOUT)
        .timeout(REQUEST_TIMEOUT)
        .build()
        .unwrap_or_default()
}

fn device_name() -> String {
    let host = fs::read_to_string("/proc/sys/kernel/hostname")
        .ok()
        .map(|value| value.trim().to_string())
        .filter(|value| !value.is_empty())
        .or_else(|| std::env::var("HOSTNAME").ok());
    match host {
        Some(value) => format!("Dopamine Desktop ({value})"),
        None => String::from("Dopamine Desktop"),
    }
}

pub async fn ping(base_url: &str) -> bool {
    let url = format!("{base_url}/api/v1/ping");
    match http().get(url).send().await {
        Ok(response) => response.status().is_success(),
        Err(_) => false,
    }
}

pub async fn login(
    base_url: &str,
    username: &str,
    password: &str,
) -> AuthOutcome {
    let body = json!({
        "username": username,
        "password": password,
        "device_name": device_name(),
        "client_type": "desktop",
    });
    match http()
        .post(format!("{base_url}/api/v1/auth/login"))
        .json(&body)
        .send()
        .await
    {
        Ok(response) => finish(response, base_url, "").await,
        Err(error) => AuthOutcome::network(error),
    }
}

pub async fn consume_qr(base_url: &str, qr_token: &str) -> AuthOutcome {
    let body = json!({
        "qr_token": qr_token,
        "device_name": device_name(),
        "client_type": "desktop",
    });
    match http()
        .post(format!("{base_url}/api/v1/auth/qr/consume"))
        .json(&body)
        .send()
        .await
    {
        Ok(response) => finish(response, base_url, "").await,
        Err(error) => AuthOutcome::network(error),
    }
}

pub async fn validate(base_url: &str, token: &str) -> AuthOutcome {
    let mut req = http().get(format!("{base_url}/api/v1/auth/me"));
    if !token.trim().is_empty() {
        req = req.bearer_auth(token);
    }
    match req.send().await {
        Ok(response) => finish(response, base_url, token).await,
        Err(error) => AuthOutcome::network(error),
    }
}

pub async fn logout(base_url: &str, token: &str) {
    let _ = http()
        .post(format!("{base_url}/api/v1/auth/logout"))
        .bearer_auth(token)
        .json(&json!({}))
        .send()
        .await;
}

async fn finish(
    response: reqwest::Response,
    base_url: &str,
    token: &str,
) -> AuthOutcome {
    let status = response.status().as_u16();
    let payload: Value = response.json().await.unwrap_or(Value::Null);
    let user = parse_user(&payload, base_url, token);
    let error = payload
        .get("detail")
        .and_then(Value::as_str)
        .map(str::to_string);
    AuthOutcome {
        status,
        user,
        error,
    }
}

fn parse_user(
    payload: &Value,
    base_url: &str,
    token: &str,
) -> Option<SessionUser> {
    let raw = payload.get("user")?;
    let username = raw.get("username")?.as_str()?.to_string();
    if username.is_empty() {
        return None;
    }
    let display_name = raw
        .get("display_name")
        .and_then(Value::as_str)
        .filter(|value| !value.trim().is_empty())
        .unwrap_or(&username)
        .to_string();
    let avatar = raw
        .get("avatar_url")
        .and_then(Value::as_str)
        .unwrap_or_default();
    let avatar_url = if avatar.starts_with('/') {
        format!("{base_url}{avatar}")
    } else {
        avatar.to_string()
    };
    let token = if token.is_empty() {
        payload
            .get("token")
            .and_then(Value::as_str)
            .unwrap_or_default()
            .to_string()
    } else {
        token.to_string()
    };
    let is_anonymous = payload
        .get("legacy_anonymous")
        .and_then(Value::as_bool)
        .unwrap_or(false);
    if token.is_empty() && !is_anonymous {
        return None;
    }
    Some(SessionUser {
        id: raw.get("id").and_then(Value::as_i64).unwrap_or(0),
        server_url: base_url.to_string(),
        username,
        display_name,
        email: raw
            .get("email")
            .and_then(Value::as_str)
            .unwrap_or_default()
            .to_string(),
        avatar_url,
        session_id: payload
            .get("session_id")
            .and_then(Value::as_str)
            .unwrap_or_default()
            .to_string(),
        token,
        is_logged_in: true,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_login_payload_and_avatar() {
        let payload = json!({
            "token": "abc.def",
            "session_id": "sess_1",
            "user": {
                "id": 7,
                "username": "ismael",
                "display_name": "Ismael",
                "email": "ismael@dopamine.fm",
                "avatar_url": "/api/v1/users/avatar/ismael"
            }
        });
        let user = parse_user(&payload, "http://127.0.0.1:8000", "")
            .expect("user");
        assert_eq!(user.id, 7);
        assert_eq!(user.token, "abc.def");
        assert_eq!(
            user.avatar_url,
            "http://127.0.0.1:8000/api/v1/users/avatar/ismael"
        );
        assert!(user.is_logged_in);
    }

    #[test]
    fn keeps_existing_token_when_validating() {
        let payload = json!({
            "session_id": "sess_2",
            "user": { "id": 0, "username": "listener" }
        });
        let user = parse_user(&payload, "http://host", "cached")
            .expect("user");
        assert_eq!(user.token, "cached");
        assert_eq!(user.display_name, "listener");
    }

    #[test]
    fn rejects_payload_without_user_or_token() {
        assert!(parse_user(&json!({}), "http://host", "token").is_none());
        assert!(parse_user(
            &json!({ "user": { "username": "other" } }),
            "http://host",
            ""
        )
        .is_none());
    }

    #[test]
    fn parses_legacy_anonymous_without_token() {
        let payload = json!({
            "authenticated": true,
            "legacy_anonymous": true,
            "user": {
                "id": 0,
                "username": "listener",
                "display_name": "Dopamine Listener",
                "avatar_url": "/api/v1/users/avatar/listener"
            }
        });
        let user = parse_user(&payload, "http://127.0.0.1:8000", "").expect("anon user");
        assert_eq!(user.id, 0);
        assert_eq!(user.username, "listener");
        assert_eq!(user.token, "");
    }
}
