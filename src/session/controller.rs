// Location: src/session/controller.rs
// Function: Pure Rust session controller driving server connectivity, login,
// logout, QR/deep-link handoff adoption, and asynchronous state updates.
// All Rights Reserved isgaar

use std::sync::mpsc::{self, Receiver, Sender};

use super::api;
use super::handoff::{parse_handoff, HandoffKind};
use super::model::{normalized_base_url, server_host, SessionUser};
use super::store::{default_server_draft, SessionStore};

#[derive(Debug, Clone)]
pub enum SessionEvent {
    ServerAdopted(String),
    LoginSucceeded(SessionUser),
    LoggedOut,
    SessionWindowRequested,
    WindowRaiseRequested,
    StateChanged,
}

#[derive(Debug)]
enum WorkerMessage {
    Connected {
        generation: u64,
        base_url: String,
        online: bool,
        outcome: Option<api::AuthOutcome>,
    },
    AuthFinished {
        generation: u64,
        base_url: String,
        outcome: api::AuthOutcome,
    },
    ValidationFinished {
        generation: u64,
        base_url: String,
        outcome: api::AuthOutcome,
    },
}

pub struct SessionController {
    store: SessionStore,
    pub user: SessionUser,
    pub server_draft: String,
    pub server_url: String,
    pub server_host: String,
    pub status_key: String,
    pub error_key: String,
    pub error_detail: String,
    pub online: bool,
    pub busy: bool,
    generation: u64,
    tx: Sender<WorkerMessage>,
    rx: Receiver<WorkerMessage>,
    events: Vec<SessionEvent>,
}

impl Default for SessionController {
    fn default() -> Self {
        Self::new(SessionStore::default())
    }
}

impl SessionController {
    pub fn new(store: SessionStore) -> Self {
        let (tx, rx) = mpsc::channel();
        let cached = store.load();
        let draft = if !cached.server_url.is_empty() {
            cached.server_url.clone()
        } else {
            default_server_draft()
        };
        let normalized = normalized_base_url(&draft).unwrap_or(draft);
        let host = server_host(&normalized);

        let mut ctrl = Self {
            store,
            user: cached.clone(),
            server_draft: normalized.clone(),
            server_url: normalized.clone(),
            server_host: host,
            status_key: if cached.is_logged_in {
                String::from("session_status_active")
            } else {
                String::from("session_status_disconnected")
            },
            error_key: String::new(),
            error_detail: String::new(),
            online: cached.is_logged_in,
            busy: false,
            generation: 0,
            tx,
            rx,
            events: Vec::new(),
        };

        if cached.is_logged_in {
            ctrl.validate_cached(&normalized, &cached.token);
        }
        ctrl
    }

    pub fn drain_events(&mut self) -> Vec<SessionEvent> {
        self.poll_worker();
        std::mem::take(&mut self.events)
    }

    pub fn poll_worker(&mut self) {
        while let Ok(msg) = self.rx.try_recv() {
            match msg {
                WorkerMessage::Connected {
                    generation,
                    base_url,
                    online,
                    outcome,
                } => {
                    if generation != self.generation {
                        continue;
                    }
                    self.busy = false;
                    self.online = online;
                    if !online {
                        self.status_key = String::from("session_status_offline");
                        self.error_key = String::from("session_err_unreachable");
                        self.events.push(SessionEvent::StateChanged);
                        continue;
                    }
                    self.server_url = base_url.clone();
                    self.server_host = server_host(&base_url);
                    self.events.push(SessionEvent::ServerAdopted(base_url));
                    if let Some(auth) = outcome {
                        if auth.ok() {
                            if let Some(user) = auth.user {
                                self.commit_user(user);
                                continue;
                            }
                        }
                    }
                    self.status_key = String::from("session_status_server_ready");
                    self.events.push(SessionEvent::StateChanged);
                }
                WorkerMessage::AuthFinished {
                    generation,
                    base_url,
                    outcome,
                } => {
                    if generation != self.generation {
                        continue;
                    }
                    self.busy = false;
                    if outcome.ok() {
                        self.online = true;
                        self.server_url = base_url.clone();
                        self.server_host = server_host(&base_url);
                        self.events.push(SessionEvent::ServerAdopted(base_url));
                        if let Some(user) = outcome.user {
                            self.commit_user(user);
                        }
                    } else {
                        self.online = outcome.status != 0;
                        self.error_detail = outcome.error.unwrap_or_default();
                        self.error_key = match outcome.status {
                            0 => String::from("session_err_unreachable"),
                            401 | 403 => String::from("session_err_credentials"),
                            _ => String::from("session_err_generic"),
                        };
                        self.status_key = String::from("session_status_error");
                        self.events.push(SessionEvent::StateChanged);
                    }
                }
                WorkerMessage::ValidationFinished {
                    generation,
                    base_url,
                    outcome,
                } => {
                    if generation != self.generation {
                        continue;
                    }
                    self.busy = false;
                    if outcome.ok() {
                        self.online = true;
                        self.server_url = base_url;
                        self.server_host = server_host(&self.server_url);
                        if let Some(user) = outcome.user {
                            self.commit_user(user);
                        }
                    } else if outcome.status == 401 || outcome.status == 403 {
                        self.clear_session();
                    } else {
                        self.online = false;
                        self.status_key = String::from("session_status_offline");
                        self.events.push(SessionEvent::StateChanged);
                    }
                }
            }
        }
    }

    pub fn connect_server(&mut self, raw_url: &str) {
        let Some(base_url) = normalized_base_url(raw_url) else {
            self.error_key = String::from("session_err_invalid_url");
            self.events.push(SessionEvent::StateChanged);
            return;
        };
        self.server_draft = base_url.clone();
        self.error_key.clear();
        self.error_detail.clear();
        self.busy = true;
        self.status_key = String::from("session_status_connecting");
        self.generation += 1;
        let generation = self.generation;
        let token = self.user.token.clone();
        let tx = self.tx.clone();

        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let (online, outcome) = rt.block_on(async {
                let ok = api::ping(&base_url).await;
                let auth = if ok {
                    Some(api::validate(&base_url, &token).await)
                } else {
                    None
                };
                (ok, auth)
            });
            let _ = tx.send(WorkerMessage::Connected {
                generation,
                base_url,
                online,
                outcome,
            });
        });
    }

    pub fn login(&mut self, raw_url: &str, username: &str, password: &str) {
        let Some(base_url) = normalized_base_url(raw_url) else {
            self.error_key = String::from("session_err_invalid_url");
            self.events.push(SessionEvent::StateChanged);
            return;
        };
        let username = username.trim().to_string();
        let password = password.to_string();
        if username.is_empty() || password.is_empty() {
            self.error_key = String::from("session_err_credentials");
            self.events.push(SessionEvent::StateChanged);
            return;
        }
        self.server_draft = base_url.clone();
        self.error_key.clear();
        self.error_detail.clear();
        self.busy = true;
        self.status_key = String::from("session_status_signing_in");
        self.generation += 1;
        let generation = self.generation;
        let tx = self.tx.clone();

        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let outcome = rt.block_on(api::login(&base_url, &username, &password));
            let _ = tx.send(WorkerMessage::AuthFinished {
                generation,
                base_url,
                outcome,
            });
        });
    }

    pub fn logout(&mut self) {
        let base_url = self.server_url.clone();
        let token = self.user.token.clone();
        if !base_url.is_empty() && !token.is_empty() {
            std::thread::spawn(move || {
                if let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                    .enable_all()
                    .build()
                {
                    rt.block_on(api::logout(&base_url, &token));
                }
            });
        }
        self.clear_session();
    }

    pub fn handle_uri(&mut self, uri: &str) -> bool {
        let link = parse_handoff(uri);
        if !link.is_valid() {
            return false;
        }
        self.events.push(SessionEvent::WindowRaiseRequested);
        let target_server = if !link.server_url.is_empty() {
            link.server_url
        } else {
            self.server_url.clone()
        };
        let Some(base_url) = normalized_base_url(&target_server) else {
            self.error_key = String::from("session_err_invalid_url");
            self.events.push(SessionEvent::SessionWindowRequested);
            return false;
        };
        self.server_draft = base_url.clone();
        self.server_url = base_url.clone();
        self.server_host = server_host(&base_url);

        match link.kind {
            Some(HandoffKind::Qr) if !link.token.is_empty() => {
                self.consume_qr(&base_url, &link.token);
                true
            }
            Some(HandoffKind::Connect) => {
                if !link.token.is_empty() {
                    self.validate_cached(&base_url, &link.token);
                } else {
                    self.connect_server(&base_url);
                }
                true
            }
            _ => false,
        }
    }

    fn consume_qr(&mut self, base_url: &str, qr_token: &str) {
        self.busy = true;
        self.error_key.clear();
        self.status_key = String::from("session_status_handoff");
        self.generation += 1;
        let generation = self.generation;
        let base_url = base_url.to_string();
        let qr_token = qr_token.to_string();
        let tx = self.tx.clone();

        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let outcome = rt.block_on(api::consume_qr(&base_url, &qr_token));
            let _ = tx.send(WorkerMessage::AuthFinished {
                generation,
                base_url,
                outcome,
            });
        });
    }

    fn validate_cached(&mut self, base_url: &str, token: &str) {
        self.busy = true;
        self.generation += 1;
        let generation = self.generation;
        let base_url = base_url.to_string();
        let token = token.to_string();
        let tx = self.tx.clone();

        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let outcome = rt.block_on(api::validate(&base_url, &token));
            let _ = tx.send(WorkerMessage::ValidationFinished {
                generation,
                base_url,
                outcome,
            });
        });
    }

    fn commit_user(&mut self, user: SessionUser) {
        self.store.save(&user);
        self.user = user.clone();
        self.status_key = String::from("session_status_active");
        self.error_key.clear();
        self.error_detail.clear();
        self.events.push(SessionEvent::LoginSucceeded(user));
        self.events.push(SessionEvent::StateChanged);
    }

    fn clear_session(&mut self) {
        self.generation += 1;
        self.store.clear();
        self.user = SessionUser::default();
        self.busy = false;
        self.status_key = String::from("session_status_disconnected");
        self.error_key.clear();
        self.error_detail.clear();
        self.events.push(SessionEvent::LoggedOut);
        self.events.push(SessionEvent::StateChanged);
    }
}

