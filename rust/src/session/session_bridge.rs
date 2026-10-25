// Location: rust/src/session/session_bridge.rs
// Function: cxx-qt bridge exposing the Rust session state, auth actions, and
// web handoff signals to the QML presentation layer.
// All Rights Reserved isgaar

#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    extern "RustQt" {
        #[qobject]
        #[namespace = "dopamine::rust"]
        #[qproperty(bool, is_logged_in, cxx_name = "isLoggedIn")]
        #[qproperty(bool, busy)]
        #[qproperty(bool, server_online, cxx_name = "serverOnline")]
        #[qproperty(QString, server_status, cxx_name = "serverStatus")]
        #[qproperty(QString, server_draft, cxx_name = "serverDraft")]
        #[qproperty(QString, server_url, cxx_name = "serverUrl")]
        #[qproperty(QString, server_host, cxx_name = "serverHost")]
        #[qproperty(QString, username)]
        #[qproperty(QString, display_name, cxx_name = "displayName")]
        #[qproperty(QString, email)]
        #[qproperty(QString, avatar_url, cxx_name = "avatarUrl")]
        #[qproperty(QString, initial)]
        #[qproperty(QString, error_key, cxx_name = "errorKey")]
        type SessionBridge = super::SessionBridgeRust;

        #[qinvokable]
        fn initialize(self: Pin<&mut Self>);

        #[qinvokable]
        #[cxx_name = "checkServer"]
        fn check_server(self: Pin<&mut Self>);

        #[qinvokable]
        fn login(
            self: Pin<&mut Self>,
            username: &QString,
            password: &QString,
        );

        #[qinvokable]
        fn logout(self: Pin<&mut Self>);

        #[qinvokable]
        #[cxx_name = "handleUri"]
        fn handle_uri(self: Pin<&mut Self>, uri: &QString);

        #[qinvokable]
        #[cxx_name = "clearError"]
        fn clear_error(self: Pin<&mut Self>);

        #[qsignal]
        #[cxx_name = "loginSucceeded"]
        fn login_succeeded(self: Pin<&mut Self>);

        #[qsignal]
        #[cxx_name = "handoffCompleted"]
        fn handoff_completed(self: Pin<&mut Self>);

        #[qsignal]
        #[cxx_name = "sessionWindowRequested"]
        fn session_window_requested(self: Pin<&mut Self>);

        #[qsignal]
        #[cxx_name = "serverAdopted"]
        fn server_adopted(self: Pin<&mut Self>, url: &QString);

        #[qsignal]
        #[cxx_name = "bearerTokenChanged"]
        fn bearer_token_changed(self: Pin<&mut Self>, token: &QString);
    }

    impl cxx_qt::Threading for SessionBridge {}
}

use std::pin::Pin;

use cxx_qt::{CxxQtType, Threading};
use cxx_qt_lib::QString;

use super::api::{self, AuthOutcome};
use super::handoff::{self, HandoffKind};
use super::model::{self, SessionUser};
use super::store::{self, SessionStore};
use crate::bridge::run_blocking;

pub struct SessionBridgeRust {
    is_logged_in: bool,
    busy: bool,
    server_online: bool,
    server_status: QString,
    server_draft: QString,
    server_url: QString,
    server_host: QString,
    username: QString,
    display_name: QString,
    email: QString,
    avatar_url: QString,
    initial: QString,
    error_key: QString,
    token: String,
    generation: u64,
    store: SessionStore,
}

impl Default for SessionBridgeRust {
    fn default() -> Self {
        let store = SessionStore::default();
        let user = store.load();
        let draft = initial_draft(&user);
        let status = if user.is_logged_in {
            "checking"
        } else {
            "unknown"
        };
        Self {
            is_logged_in: user.is_logged_in,
            busy: false,
            server_online: false,
            server_status: QString::from(status),
            server_draft: QString::from(draft.as_str()),
            server_url: QString::from(user.server_url.as_str()),
            server_host: QString::from(model::server_host(&draft).as_str()),
            username: QString::from(user.username.as_str()),
            display_name: QString::from(user.display().as_str()),
            email: QString::from(user.email.as_str()),
            avatar_url: QString::from(user.avatar_url.as_str()),
            initial: QString::from(user.initial().as_str()),
            error_key: QString::default(),
            token: user.token,
            generation: 0,
            store,
        }
    }
}

impl qobject::SessionBridge {
    pub fn initialize(mut self: Pin<&mut Self>) {
        let token = self.rust().token.clone();
        self.as_mut()
            .bearer_token_changed(&QString::from(token.as_str()));
        if !*self.is_logged_in() {
            if !self.server_draft().is_empty() {
                self.as_mut().check_server();
            }
            return;
        }
        let Some(base) = model::normalized_base_url(
            &self.server_url().to_string(),
        ) else {
            apply_status(self.as_mut(), "offline");
            return;
        };
        apply_status(self.as_mut(), "checking");
        let generation = next_generation(self.as_mut());
        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let outcome = complete(run_blocking(api::validate(&base, &token)));
            let _ = qt_thread.queue(move |mut object| {
                if object.rust().generation != generation {
                    return;
                }
                if outcome.ok() {
                    if let Some(mut user) = outcome.user {
                        user.token = object.rust().token.clone();
                        commit_user(object.as_mut(), user);
                        clear_error(object.as_mut());
                    }
                    apply_status(object.as_mut(), "online");
                } else if outcome.status == 401 {
                    clear_session(object.as_mut());
                    apply_status(object.as_mut(), "online");
                    object
                        .as_mut()
                        .set_error_key(QString::from("session_expired"));
                } else {
                    apply_status(object.as_mut(), "offline");
                }
            });
        });
    }

    pub fn check_server(mut self: Pin<&mut Self>) {
        let Some(base) = model::normalized_base_url(
            &self.server_draft().to_string(),
        ) else {
            apply_status(self.as_mut(), "offline");
            self.as_mut()
                .set_error_key(QString::from("session_invalid_url"));
            return;
        };
        apply_status(self.as_mut(), "checking");
        let generation = self.rust().generation;
        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let online = run_blocking(async { api::ping(&base).await })
                .unwrap_or(false);
            let _ = qt_thread.queue(move |mut object| {
                if object.rust().generation != generation {
                    return;
                }
                apply_status(
                    object.as_mut(),
                    if online { "online" } else { "offline" },
                );
                if online
                    && object.error_key().to_string() == "session_unreachable"
                {
                    clear_error(object.as_mut());
                }
            });
        });
    }

    pub fn login(
        mut self: Pin<&mut Self>,
        username: &QString,
        password: &QString,
    ) {
        clear_error(self.as_mut());
        let Some(base) = model::normalized_base_url(
            &self.server_draft().to_string(),
        ) else {
            self.as_mut()
                .set_error_key(QString::from("session_invalid_url"));
            self.as_mut().session_window_requested();
            return;
        };
        let username = username.to_string().trim().to_string();
        if username.is_empty() {
            self.as_mut()
                .set_error_key(QString::from("session_missing_username"));
            return;
        }
        let password = password.to_string();
        self.as_mut().set_busy(true);
        apply_status(self.as_mut(), "checking");
        let generation = next_generation(self.as_mut());
        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let outcome = complete(run_blocking(api::login(
                &base, &username, &password,
            )));
            let _ = qt_thread.queue(move |mut object| {
                if object.rust().generation != generation {
                    return;
                }
                object.as_mut().set_busy(false);
                if outcome.ok() {
                    if let Some(user) = outcome.user {
                        object
                            .as_mut()
                            .set_server_draft(QString::from(base.as_str()));
                        object
                            .as_mut()
                            .server_adopted(&QString::from(base.as_str()));
                        commit_user(object.as_mut(), user);
                    }
                    apply_status(object.as_mut(), "online");
                    clear_error(object.as_mut());
                    object.as_mut().login_succeeded();
                } else {
                    apply_status(
                        object.as_mut(),
                        if outcome.status == 0 {
                            "offline"
                        } else {
                            "online"
                        },
                    );
                    let key = if outcome.status == 401 {
                        "session_bad_credentials"
                    } else if outcome.status == 0 {
                        "session_unreachable"
                    } else {
                        "session_login_failed"
                    };
                    object.as_mut().set_error_key(QString::from(key));
                }
            });
        });
    }

    pub fn logout(mut self: Pin<&mut Self>) {
        let token = self.rust().token.clone();
        if !*self.is_logged_in() && token.is_empty() {
            clear_error(self.as_mut());
            return;
        }
        let base = model::normalized_base_url(
            &self.server_url().to_string(),
        );
        next_generation(self.as_mut());
        if let Some(base) = base {
            if !token.is_empty() {
                std::thread::spawn(move || {
                    let _ = run_blocking(async {
                        api::logout(&base, &token).await
                    });
                });
            }
        }
        clear_session(self.as_mut());
        clear_error(self.as_mut());
    }

    pub fn handle_uri(mut self: Pin<&mut Self>, uri: &QString) {
        let link = handoff::parse_handoff(&uri.to_string());
        if !link.is_valid() {
            return;
        }
        clear_error(self.as_mut());
        if link.kind == Some(HandoffKind::Connect) {
            let Some(base) =
                model::normalized_base_url(&link.server_url)
            else {
                return;
            };
            self.as_mut()
                .set_server_draft(QString::from(base.as_str()));
            self.as_mut()
                .server_adopted(&QString::from(base.as_str()));

            let generation = next_generation(self.as_mut());
            let qt_thread = self.qt_thread();
            let token = link.token.clone();
            self.as_mut().set_busy(true);
            apply_status(self.as_mut(), "checking");
            std::thread::spawn(move || {
                let outcome = complete(run_blocking(api::validate(&base, &token)));
                let _ = qt_thread.queue(move |mut object| {
                    if object.rust().generation != generation {
                        return;
                    }
                    object.as_mut().set_busy(false);
                    if outcome.ok() {
                        if let Some(user) = outcome.user {
                            commit_user(object.as_mut(), user);
                        }
                        apply_status(object.as_mut(), "online");
                        clear_error(object.as_mut());
                        object.as_mut().handoff_completed();
                        object.as_mut().login_succeeded();
                    } else {
                        apply_status(
                            object.as_mut(),
                            if outcome.status == 0 {
                                "offline"
                            } else {
                                "online"
                            },
                        );
                        object.as_mut().session_window_requested();
                    }
                });
            });
            return;
        }
        let server = if link.server_url.is_empty() {
            self.server_draft().to_string()
        } else {
            link.server_url.clone()
        };
        let Some(base) = model::normalized_base_url(&server) else {
            self.as_mut()
                .set_error_key(QString::from("session_qr_invalid"));
            self.as_mut().session_window_requested();
            return;
        };
        if link.token.is_empty() {
            self.as_mut()
                .set_error_key(QString::from("session_qr_invalid"));
            self.as_mut().session_window_requested();
            return;
        }
        self.as_mut().set_busy(true);
        apply_status(self.as_mut(), "checking");
        let generation = next_generation(self.as_mut());
        let qt_thread = self.qt_thread();
        let qr_token = link.token;
        std::thread::spawn(move || {
            let outcome = complete(run_blocking(api::consume_qr(
                &base, &qr_token,
            )));
            let _ = qt_thread.queue(move |mut object| {
                if object.rust().generation != generation {
                    return;
                }
                object.as_mut().set_busy(false);
                if outcome.ok() {
                    if let Some(user) = outcome.user {
                        object
                            .as_mut()
                            .set_server_draft(QString::from(base.as_str()));
                        object
                            .as_mut()
                            .server_adopted(&QString::from(base.as_str()));
                        commit_user(object.as_mut(), user);
                    }
                    apply_status(object.as_mut(), "online");
                    clear_error(object.as_mut());
                    object.as_mut().handoff_completed();
                    object.as_mut().login_succeeded();
                } else {
                    apply_status(
                        object.as_mut(),
                        if outcome.status == 0 {
                            "offline"
                        } else {
                            "online"
                        },
                    );
                    let key = if outcome.status == 0 {
                        "session_unreachable"
                    } else {
                        "session_qr_invalid"
                    };
                    object.as_mut().set_error_key(QString::from(key));
                }
                object.as_mut().session_window_requested();
            });
        });
    }

    pub fn clear_error(mut self: Pin<&mut Self>) {
        clear_error(self.as_mut());
    }
}

fn initial_draft(user: &SessionUser) -> String {
    let raw = if user.server_url.is_empty() {
        store::default_server_draft()
    } else {
        user.server_url.clone()
    };
    model::normalized_base_url(&raw)
        .unwrap_or_else(|| String::from("http://127.0.0.1:8000"))
}

fn next_generation(mut object: Pin<&mut qobject::SessionBridge>) -> u64 {
    let generation = object.rust().generation.wrapping_add(1);
    object.as_mut().rust_mut().generation = generation;
    generation
}

fn complete(outcome: Result<AuthOutcome, String>) -> AuthOutcome {
    outcome.unwrap_or_else(|error| AuthOutcome {
        status: 0,
        user: None,
        error: Some(error),
    })
}

fn apply_status(object: Pin<&mut qobject::SessionBridge>, status: &str) {
    let mut object = object;
    object
        .as_mut()
        .set_server_status(QString::from(status));
    object
        .as_mut()
        .set_server_online(status == "online");
}

fn clear_error(mut object: Pin<&mut qobject::SessionBridge>) {
    if !object.error_key().is_empty() {
        object.as_mut().set_error_key(QString::default());
    }
}

fn commit_user(
    mut object: Pin<&mut qobject::SessionBridge>,
    user: SessionUser,
) {
    let token = user.token.clone();
    let display = user.display();
    let initial = user.initial();
    {
        let mut rust = object.as_mut().rust_mut();
        rust.token = token.clone();
        rust.store.save(&user);
    }
    object.as_mut().set_is_logged_in(true);
    object
        .as_mut()
        .set_server_url(QString::from(user.server_url.as_str()));
    object
        .as_mut()
        .set_server_host(QString::from(
            model::server_host(&user.server_url).as_str(),
        ));
    object
        .as_mut()
        .set_server_draft(QString::from(user.server_url.as_str()));
    object
        .as_mut()
        .set_username(QString::from(user.username.as_str()));
    object
        .as_mut()
        .set_display_name(QString::from(display.as_str()));
    object
        .as_mut()
        .set_email(QString::from(user.email.as_str()));
    object
        .as_mut()
        .set_avatar_url(QString::from(user.avatar_url.as_str()));
    object
        .as_mut()
        .set_initial(QString::from(initial.as_str()));
    apply_status(object.as_mut(), "online");
    object
        .as_mut()
        .bearer_token_changed(&QString::from(token.as_str()));
}

fn clear_session(mut object: Pin<&mut qobject::SessionBridge>) {
    object.as_mut().rust_mut().store.clear();
    object.as_mut().rust_mut().token.clear();
    object.as_mut().set_is_logged_in(false);
    object.as_mut().set_server_url(QString::default());
    object.as_mut().set_server_host(QString::default());
    object.as_mut().set_username(QString::default());
    object.as_mut().set_display_name(QString::default());
    object.as_mut().set_email(QString::default());
    object.as_mut().set_avatar_url(QString::default());
    object.as_mut().set_initial(QString::default());
    object
        .as_mut()
        .bearer_token_changed(&QString::default());
}
