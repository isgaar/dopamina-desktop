// Location: rust/src/session/handoff.rs
// Function: Strict parser for dopamine://auth/qr and dopamine://connect
// links used by the web console to hand off a session.
// All Rights Reserved isgaar

use url::Url;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HandoffKind {
    Qr,
    Connect,
}

#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct HandoffLink {
    pub kind: Option<HandoffKind>,
    pub server_url: String,
    pub token: String,
}

impl HandoffLink {
    pub fn is_valid(&self) -> bool {
        self.kind.is_some()
    }
}

pub fn parse_handoff(value: &str) -> HandoffLink {
    let mut link = HandoffLink::default();
    let Ok(url) = Url::parse(value.trim()) else {
        return link;
    };
    if url.scheme() != "dopamine" {
        return link;
    }
    let host = url.host_str().unwrap_or_default().to_lowercase();
    let path = url.path().to_lowercase();
    if host == "connect" {
        link.kind = Some(HandoffKind::Connect);
    } else if host == "qr" || (host == "auth" && path == "/qr") {
        link.kind = Some(HandoffKind::Qr);
    } else {
        return HandoffLink::default();
    }
    for (key, value) in url.query_pairs() {
        match key.as_ref() {
            "server" => link.server_url = value.into_owned(),
            "token" => link.token = value.into_owned(),
            _ => {}
        }
    }
    link
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_qr_handoff() {
        let link = parse_handoff(
            "dopamine://auth/qr?token=abc.def\
             &server=http%3A%2F%2F127.0.0.1%3A8000",
        );
        assert!(link.is_valid());
        assert_eq!(link.kind, Some(HandoffKind::Qr));
        assert_eq!(link.token, "abc.def");
        assert_eq!(link.server_url, "http://127.0.0.1:8000");
    }

    #[test]
    fn parses_connect_handoff() {
        let link = parse_handoff(
            "dopamine://connect?server=https%3A%2F%2Fmusic.example.com",
        );
        assert_eq!(link.kind, Some(HandoffKind::Connect));
        assert_eq!(link.server_url, "https://music.example.com");
        assert!(link.token.is_empty());
    }

    #[test]
    fn rejects_foreign_or_unknown_links() {
        assert!(!parse_handoff(
            "https://example.com/?token=abc"
        )
        .is_valid());
        assert!(!parse_handoff("dopamine://unknown?server=x").is_valid());
        assert!(!parse_handoff("").is_valid());
    }
}
