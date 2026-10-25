// Location: rust/src/session/store.rs
// Function: Owner-only session cache persisting profile metadata as XML and
// the bearer token in a separate 0600 credential file.
// All Rights Reserved isgaar

use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};

use super::model::SessionUser;

#[cfg(unix)]
use std::os::unix::fs::{OpenOptionsExt, PermissionsExt};

const TOKEN_FILE: &str = "session.token";
const CACHE_FILE: &str = "session.xml";

pub struct SessionStore {
    directory: PathBuf,
}

impl Default for SessionStore {
    fn default() -> Self {
        Self::at(default_directory())
    }
}

impl SessionStore {
    pub fn at(directory: PathBuf) -> Self {
        Self { directory }
    }

    pub fn cache_path(&self) -> PathBuf {
        self.directory.join(CACHE_FILE)
    }

    pub fn token_path(&self) -> PathBuf {
        self.directory.join("credentials").join(TOKEN_FILE)
    }

    pub fn load(&self) -> SessionUser {
        let mut user = SessionUser::default();
        if let Ok(xml) = fs::read_to_string(self.cache_path()) {
            user.server_url = value_of(&xml, "server").unwrap_or_default();
            user.id = value_of(&xml, "user_id")
                .and_then(|value| value.parse().ok())
                .unwrap_or(0);
            user.username = value_of(&xml, "username").unwrap_or_default();
            user.display_name =
                value_of(&xml, "display_name").unwrap_or_default();
            user.email = value_of(&xml, "email").unwrap_or_default();
            user.avatar_url =
                value_of(&xml, "avatar_url").unwrap_or_default();
            user.session_id =
                value_of(&xml, "session_id").unwrap_or_default();
            user.is_logged_in =
                value_of(&xml, "logged_in").as_deref() == Some("true");
        }
        user.token = self.token();
        if user.token.is_empty() {
            user.is_logged_in = false;
        }
        user
    }

    pub fn save(&self, user: &SessionUser) {
        let xml = format!(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
             <session>\n\
             \x20 <server>{}</server>\n\
             \x20 <user_id>{}</user_id>\n\
             \x20 <username>{}</username>\n\
             \x20 <display_name>{}</display_name>\n\
             \x20 <email>{}</email>\n\
             \x20 <avatar_url>{}</avatar_url>\n\
             \x20 <session_id>{}</session_id>\n\
             \x20 <logged_in>{}</logged_in>\n\
             </session>\n",
            escape(&user.server_url),
            user.id,
            escape(&user.username),
            escape(&user.display_name),
            escape(&user.email),
            escape(&user.avatar_url),
            escape(&user.session_id),
            if user.is_logged_in { "true" } else { "false" }
        );
        let _ = write_private(&self.cache_path(), xml.as_bytes());
        if user.is_logged_in && !user.token.is_empty() {
            self.save_token(&user.token);
        } else {
            self.delete_token();
        }
    }

    pub fn clear(&self) {
        let _ = fs::remove_file(self.cache_path());
        self.delete_token();
    }

    pub fn token(&self) -> String {
        fs::read_to_string(self.token_path())
            .map(|value| value.trim().to_string())
            .unwrap_or_default()
    }

    pub fn save_token(&self, token: &str) {
        let _ = write_private(&self.token_path(), token.as_bytes());
    }

    pub fn delete_token(&self) {
        let _ = fs::remove_file(self.token_path());
    }
}

fn write_private(path: &Path, content: &[u8]) -> std::io::Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
        #[cfg(unix)]
        fs::set_permissions(parent, fs::Permissions::from_mode(0o700))?;
    }
    let temporary = path.with_extension("tmp");
    {
        let mut options = OpenOptions::new();
        options.write(true).create(true).truncate(true);
        #[cfg(unix)]
        options.mode(0o600);
        let mut file = options.open(&temporary)?;
        file.write_all(content)?;
        file.sync_all()?;
    }
    fs::rename(&temporary, path)?;
    #[cfg(unix)]
    fs::set_permissions(path, fs::Permissions::from_mode(0o600))?;
    Ok(())
}

fn value_of(xml: &str, tag: &str) -> Option<String> {
    let open = format!("<{tag}>");
    let close = format!("</{tag}>");
    let start = xml.find(&open)? + open.len();
    let end = xml[start..].find(&close)? + start;
    Some(unescape(&xml[start..end]))
}

fn escape(value: &str) -> String {
    value
        .replace('&', "&amp;")
        .replace('<', "&lt;")
        .replace('>', "&gt;")
        .replace('"', "&quot;")
        .replace('\'', "&apos;")
}

fn unescape(value: &str) -> String {
    value
        .replace("&lt;", "<")
        .replace("&gt;", ">")
        .replace("&quot;", "\"")
        .replace("&apos;", "'")
        .replace("&amp;", "&")
}

fn default_directory() -> PathBuf {
    config_root().join("isgaar").join("Dopamine")
}

fn config_root() -> PathBuf {
    if let Ok(base) = std::env::var("XDG_CONFIG_HOME") {
        if !base.trim().is_empty() {
            return PathBuf::from(base);
        }
    }
    if let Ok(home) = std::env::var("HOME") {
        if !home.trim().is_empty() {
            return PathBuf::from(home).join(".config");
        }
    }
    std::env::temp_dir()
}

pub fn default_server_draft() -> String {
    if let Some(url) = settings_server_url() {
        return url;
    }
    std::env::var("DOPAMINE_SERVER_URL")
        .unwrap_or_else(|_| String::from("http://127.0.0.1:8000"))
}

fn settings_server_url() -> Option<String> {
    let path = config_root()
        .join("isgaar")
        .join("Dopamine.conf");
    let contents = fs::read_to_string(path).ok()?;
    let mut in_source = false;
    for line in contents.lines() {
        let line = line.trim();
        if line.starts_with('[') {
            in_source = line == "[Source]";
            continue;
        }
        if in_source {
            if let Some(value) = line.strip_prefix("ServerUrl=") {
                let value = value.trim();
                if !value.is_empty() {
                    return Some(value.to_string());
                }
            }
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    fn temporary_directory() -> PathBuf {
        let unique = format!(
            "dopamine-session-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .map(|time| time.as_nanos())
                .unwrap_or(0)
        );
        std::env::temp_dir().join(unique)
    }

    #[test]
    fn saves_profile_without_token_in_xml() {
        let directory = temporary_directory();
        let store = SessionStore::at(directory.clone());
        let mut user = SessionUser::default();
        user.id = 7;
        user.server_url = String::from("http://127.0.0.1:8000");
        user.username = String::from("ismael");
        user.display_name = String::from("Ismael & Co");
        user.email = String::from("ismael@dopamine.fm");
        user.session_id = String::from("sess_123");
        user.token = String::from("secret-token-value");
        user.is_logged_in = true;
        store.save(&user);

        let xml = fs::read_to_string(store.cache_path()).unwrap();
        assert!(!xml.contains("secret-token-value"));
        assert!(xml.contains("Ismael &amp; Co"));
        assert_eq!(store.token(), "secret-token-value");

        #[cfg(unix)]
        {
            let mode = fs::metadata(store.token_path())
                .unwrap()
                .permissions()
                .mode()
                & 0o777;
            assert_eq!(mode, 0o600);
        }

        let loaded = store.load();
        assert_eq!(loaded, user);
        let _ = fs::remove_dir_all(&directory);
    }

    #[test]
    fn clear_removes_profile_and_token() {
        let directory = temporary_directory();
        let store = SessionStore::at(directory.clone());
        let mut user = SessionUser::default();
        user.server_url = String::from("http://127.0.0.1:8000");
        user.username = String::from("listener");
        user.token = String::from("secret");
        user.is_logged_in = true;
        store.save(&user);
        store.clear();
        assert!(!store.cache_path().exists());
        assert!(!store.token_path().exists());
        assert!(!store.load().is_logged_in);
        let _ = fs::remove_dir_all(&directory);
    }

    #[test]
    fn escapes_and_unescapes_values() {
        let original = String::from("a < b & c > d \" e '");
        assert_eq!(unescape(&escape(&original)), original);
    }
}
