// Location: rust/src/session/model.rs
// Function: Session user model plus Core URL normalization and presentation
// helpers shared by the Rust auth client and the QML bridge.
// All Rights Reserved isgaar

use url::Url;

#[derive(Debug, Clone, Default, PartialEq)]
pub struct SessionUser {
    pub id: i64,
    pub server_url: String,
    pub username: String,
    pub display_name: String,
    pub email: String,
    pub avatar_url: String,
    pub session_id: String,
    pub token: String,
    pub is_logged_in: bool,
}

impl SessionUser {
    pub fn display(&self) -> String {
        if self.display_name.trim().is_empty() {
            self.username.clone()
        } else {
            self.display_name.clone()
        }
    }

    pub fn initial(&self) -> String {
        self.display()
            .chars()
            .next()
            .map(|letter| letter.to_uppercase().collect())
            .unwrap_or_default()
    }
}

pub fn normalized_base_url(value: &str) -> Option<String> {
    let input = value.trim();
    if input.is_empty() {
        return None;
    }
    let candidate = if input.contains("://") {
        input.to_string()
    } else {
        format!("http://{input}")
    };
    let mut url = Url::parse(&candidate).ok()?;
    let scheme = url.scheme().to_lowercase();
    if scheme != "http" && scheme != "https" {
        return None;
    }
    if url.host_str().is_none() || !url.username().is_empty() {
        return None;
    }
    if url.password().is_some() {
        return None;
    }
    url.set_query(None);
    url.set_fragment(None);
    let mut base = url.to_string();
    while base.ends_with('/') {
        base.pop();
    }
    if base.is_empty() {
        None
    } else {
        Some(base)
    }
}

pub fn server_host(base_url: &str) -> String {
    let Ok(url) = Url::parse(base_url) else {
        return base_url.to_string();
    };
    match (url.host_str(), url.port()) {
        (Some(host), Some(port)) => format!("{host}:{port}"),
        (Some(host), None) => host.to_string(),
        _ => base_url.to_string(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalizes_missing_scheme_and_trailing_slash() {
        assert_eq!(
            normalized_base_url("127.0.0.1:8000"),
            Some(String::from("http://127.0.0.1:8000"))
        );
        assert_eq!(
            normalized_base_url("http://HOST:9000/"),
            Some(String::from("http://host:9000"))
        );
    }

    #[test]
    fn strips_query_and_fragment() {
        assert_eq!(
            normalized_base_url("http://host:9000/?a=1#b"),
            Some(String::from("http://host:9000"))
        );
    }

    #[test]
    fn rejects_unsafe_or_foreign_urls() {
        assert_eq!(normalized_base_url("ftp://host"), None);
        assert_eq!(normalized_base_url("http://user:pass@host"), None);
        assert_eq!(normalized_base_url("   "), None);
    }

    #[test]
    fn derives_display_name_and_initial() {
        let mut user = SessionUser::default();
        user.username = String::from("ismael");
        assert_eq!(user.display(), "ismael");
        assert_eq!(user.initial(), "I");
        user.display_name = String::from("Ismael");
        assert_eq!(user.initial(), "I");
    }

    #[test]
    fn reports_host_with_port() {
        assert_eq!(
            server_host("http://127.0.0.1:8000"),
            "127.0.0.1:8000"
        );
    }
}
