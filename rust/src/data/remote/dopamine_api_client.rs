// Location: rust/src/data/remote/dopamine_api_client.rs
// Function: Asynchronous HTTP client for the Dopamine FastAPI backend,
// mirroring the Android DopamineApiClient catalog contract.
// All Rights Reserved isgaar

use std::fmt;
use std::time::Duration;

use serde_json::Value;

use crate::data::model::Track;

const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);
const REQUEST_TIMEOUT: Duration = Duration::from_secs(10);

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ApiError {
    Network(String),
    Status(u16),
    InvalidPayload(String),
}

impl fmt::Display for ApiError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Network(message) => write!(formatter, "network error: {message}"),
            Self::Status(code) => write!(formatter, "unexpected HTTP status: {code}"),
            Self::InvalidPayload(message) => write!(formatter, "invalid payload: {message}"),
        }
    }
}

impl std::error::Error for ApiError {}

pub struct DopamineApiClient {
    base_url: String,
    auth_token: Option<String>,
    http: reqwest::Client,
}

impl DopamineApiClient {
    pub fn new(base_url: impl Into<String>) -> Self {
        let base_url = base_url.into().trim().trim_end_matches('/').to_string();

        let http = reqwest::Client::builder()
            .connect_timeout(CONNECT_TIMEOUT)
            .timeout(REQUEST_TIMEOUT)
            .build()
            .unwrap_or_default();

        Self {
            base_url,
            auth_token: None,
            http,
        }
    }

    pub fn with_token(mut self, token: impl Into<String>) -> Self {
        self.auth_token = Some(token.into());
        self
    }

    pub fn base_url(&self) -> &str {
        &self.base_url
    }

    pub async fn check_connection(&self) -> bool {
        let url = format!("{}/", self.base_url);
        match self.http.get(url).send().await {
            Ok(response) => response.status().is_success(),
            Err(_) => false,
        }
    }

    pub async fn fetch_tracks(&self, limit: u32) -> Result<Vec<Track>, ApiError> {
        let response = self.get(&format!("/api/v1/tracks?limit={limit}")).await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;

        let items = payload.as_array().ok_or_else(|| {
            ApiError::InvalidPayload(String::from("expected a JSON array of tracks"))
        })?;

        Ok(items
            .iter()
            .filter_map(|item| Track::from_json(item, &self.base_url))
            .collect())
    }

    async fn get(&self, path: &str) -> Result<reqwest::Response, ApiError> {
        let url = format!("{}{}", self.base_url, path);
        let mut request = self.http.get(url);
        if let Some(token) = &self.auth_token {
            request = request.bearer_auth(token);
        }

        let response = request
            .send()
            .await
            .map_err(|error| ApiError::Network(error.to_string()))?;

        if !response.status().is_success() {
            return Err(ApiError::Status(response.status().as_u16()));
        }

        Ok(response)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalizes_base_url() {
        let client = DopamineApiClient::new(" http://127.0.0.1:8000/ ");
        assert_eq!(client.base_url(), "http://127.0.0.1:8000");
    }
}
