// Location: src/core/config.rs
// Function: Centralized runtime URL and music-source configuration singleton
// enforcing zero hardcoded credentials and strict URL validation.
// All Rights Reserved isgaar

use url::Url;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SourceKind {
    Server,
    Local,
    Smb,
}

#[derive(Debug, Clone)]
pub struct ProjectConfig {
    pub api_url: String,
    pub source_type: SourceKind,
    pub local_path: String,
    pub smb_host: String,
    pub smb_share: String,
    pub smb_folder: String,
    pub smb_user: String,
    pub is_production: bool,
}

impl Default for ProjectConfig {
    fn default() -> Self {
        let mut cfg = Self {
            api_url: String::new(),
            source_type: SourceKind::Server,
            local_path: String::new(),
            smb_host: String::new(),
            smb_share: String::new(),
            smb_folder: String::new(),
            smb_user: String::new(),
            is_production: !cfg!(debug_assertions),
        };
        cfg.load_environment();
        cfg
    }
}

impl ProjectConfig {
    pub fn load_environment(&mut self) {
        if let Ok(env_url) = std::env::var("DOPAMINE_API_URL")
            .or_else(|_| std::env::var("DOPAMINE_SERVER_URL"))
        {
            if let Some(valid) = validate_api_url(&env_url) {
                self.api_url = valid;
                return;
            }
        }
        if let Ok(host) = std::env::var("DOPAMINE_HOST") {
            let port = std::env::var("DOPAMINE_PORT").unwrap_or_else(|_| String::from("8000"));
            if let Some(valid) = validate_api_url(&format!("http://{}:{}", host.trim(), port.trim()))
            {
                self.api_url = valid;
                return;
            }
        }
        if !self.is_production && self.api_url.is_empty() {
            self.api_url = String::from("http://127.0.0.1:8000");
        }
    }

    pub fn set_api_url(&mut self, raw: &str) -> bool {
        if let Some(valid) = validate_api_url(raw) {
            self.api_url = valid;
            true
        } else {
            false
        }
    }

    pub fn set_server_source(&mut self, url: &str) {
        self.source_type = SourceKind::Server;
        self.local_path.clear();
        self.smb_host.clear();
        self.smb_share.clear();
        self.smb_folder.clear();
        self.smb_user.clear();
        if let Some(valid) = validate_api_url(url) {
            self.api_url = valid;
        }
    }

    pub fn set_local_source(&mut self, path: &str) {
        self.source_type = SourceKind::Local;
        self.local_path = path.trim().to_string();
        self.smb_host.clear();
        self.smb_share.clear();
        self.smb_folder.clear();
        self.smb_user.clear();
    }

    pub fn set_smb_source(&mut self, host: &str, share: &str, folder: &str, user: &str) {
        self.source_type = SourceKind::Smb;
        self.smb_host = host.trim().to_string();
        self.smb_share = share.trim().to_string();
        self.smb_folder = folder.trim().to_string();
        self.smb_user = user.trim().to_string();
    }

    pub fn is_source_configured(&self) -> bool {
        match self.source_type {
            SourceKind::Server => !self.api_url.is_empty(),
            SourceKind::Local => !self.local_path.is_empty(),
            SourceKind::Smb => !self.smb_host.is_empty() && !self.smb_share.is_empty(),
        }
    }

    pub fn target_host(&self) -> String {
        Url::parse(&self.api_url)
            .ok()
            .and_then(|u| u.host_str().map(str::to_string))
            .unwrap_or_default()
    }

    pub fn target_port(&self) -> u16 {
        Url::parse(&self.api_url)
            .ok()
            .and_then(|u| u.port_or_known_default())
            .unwrap_or(8000)
    }

    pub fn active_endpoint_label(&self) -> String {
        if self.api_url.is_empty() {
            return String::new();
        }
        format!("{}:{}", self.target_host(), self.target_port())
    }

    pub fn resolve_smb_mount_path(&self) -> Option<String> {
        if self.source_type != SourceKind::Smb || self.smb_host.is_empty() || self.smb_share.is_empty()
        {
            return None;
        }
        let uid = std::env::var("UID").unwrap_or_else(|_| String::from("1000"));
        let mut base = format!(
            "/run/user/{uid}/gvfs/smb-share:server={},share={}",
            self.smb_host, self.smb_share
        );
        if !self.smb_folder.is_empty() {
            base.push('/');
            base.push_str(self.smb_folder.trim_start_matches('/'));
        }
        Some(base)
    }
}

pub fn validate_api_url(raw: &str) -> Option<String> {
    let trimmed = raw.trim();
    if trimmed.is_empty() {
        return None;
    }
    let candidate = if trimmed.contains("://") {
        trimmed.to_string()
    } else {
        format!("http://{trimmed}")
    };
    let mut parsed = Url::parse(&candidate).ok()?;
    let scheme = parsed.scheme().to_ascii_lowercase();
    if scheme != "http" && scheme != "https" {
        return None;
    }
    if !parsed.username().is_empty() || parsed.password().is_some() {
        return None;
    }
    parsed.host_str()?;
    parsed.set_query(None);
    parsed.set_fragment(None);
    let mut normalized = parsed.to_string();
    while normalized.ends_with('/') {
        normalized.pop();
    }
    Some(normalized)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rejects_userinfo_and_foreign_schemes() {
        assert_eq!(validate_api_url("http://user:pass@127.0.0.1:8000"), None);
        assert_eq!(validate_api_url("ftp://127.0.0.1:8000"), None);
        assert_eq!(
            validate_api_url("192.168.0.2:8000/"),
            Some(String::from("http://192.168.0.2:8000"))
        );
    }
}

