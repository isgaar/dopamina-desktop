// Location: rust/src/bridge.rs
// Function: cxx-qt bridge exposing the Rust core to the existing Qt/QML
// application as a context-property QObject.
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
        #[qproperty(QString, endpoint)]
        #[qproperty(QString, status)]
        #[qproperty(QString, last)]
        #[qproperty(i32, count)]
        #[qproperty(bool, busy)]
        #[qproperty(bool, connected)]
        type CoreBridge = super::CoreBridgeRust;

        #[qinvokable]
        #[cxx_name = "setBaseUrl"]
        fn set_base_url(self: Pin<&mut Self>, base_url: &QString);

        #[qinvokable]
        #[cxx_name = "checkConnection"]
        fn check_connection(self: Pin<&mut Self>);

        #[qinvokable]
        #[cxx_name = "loadTracks"]
        fn load_tracks(self: Pin<&mut Self>);
    }

    impl cxx_qt::Threading for CoreBridge {}
}

use std::future::Future;
use std::pin::Pin;

use cxx_qt::Threading;
use cxx_qt_lib::QString;

use crate::data::model::Track;
use crate::data::remote::DopamineApiClient;

pub struct CoreBridgeRust {
    endpoint: QString,
    status: QString,
    last: QString,
    count: i32,
    busy: bool,
    connected: bool,
}

impl Default for CoreBridgeRust {
    fn default() -> Self {
        Self {
            endpoint: QString::from(default_endpoint()),
            status: QString::from("Listo"),
            last: QString::default(),
            count: 0,
            busy: false,
            connected: false,
        }
    }
}

impl qobject::CoreBridge {
    pub fn set_base_url(mut self: Pin<&mut Self>, base_url: &QString) {
        let endpoint = base_url
            .to_string()
            .trim()
            .trim_end_matches('/')
            .to_string();

        if endpoint.is_empty() || endpoint == self.endpoint().to_string() {
            return;
        }

        self.as_mut().set_endpoint(QString::from(endpoint));
        self.as_mut().set_connected(false);
        self.as_mut()
            .set_status(QString::from("Endpoint actualizado"));
    }

    pub fn check_connection(mut self: Pin<&mut Self>) {
        if *self.busy() {
            return;
        }

        let endpoint = self.endpoint().to_string();
        self.as_mut().set_busy(true);
        self.as_mut()
            .set_status(QString::from("Comprobando conexion..."));

        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let outcome =
                run_blocking(
                    async move { DopamineApiClient::new(endpoint).check_connection().await },
                );

            qt_thread
                .queue(move |mut object| {
                    object.as_mut().set_busy(false);
                    match outcome {
                        Ok(true) => {
                            object.as_mut().set_connected(true);
                            object
                                .as_mut()
                                .set_status(QString::from("Servidor disponible"));
                        }
                        Ok(false) => {
                            object.as_mut().set_connected(false);
                            object
                                .as_mut()
                                .set_status(QString::from("Servidor no disponible"));
                        }
                        Err(error) => {
                            object.as_mut().set_connected(false);
                            object
                                .as_mut()
                                .set_status(QString::from(format!("Error: {error}")));
                        }
                    }
                })
                .ok();
        });
    }

    pub fn load_tracks(mut self: Pin<&mut Self>) {
        if *self.busy() {
            return;
        }

        let endpoint = self.endpoint().to_string();
        self.as_mut().set_busy(true);
        self.as_mut()
            .set_status(QString::from("Cargando catalogo..."));

        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let outcome =
                run_blocking(
                    async move { DopamineApiClient::new(endpoint).fetch_tracks(2500).await },
                );

            qt_thread
                .queue(move |mut object| {
                    object.as_mut().set_busy(false);
                    match outcome {
                        Ok(Ok(tracks)) => apply_tracks(object.as_mut(), tracks),
                        Ok(Err(error)) => {
                            object.as_mut().set_connected(false);
                            object
                                .as_mut()
                                .set_status(QString::from(format!("Error de API: {error}")));
                        }
                        Err(error) => {
                            object
                                .as_mut()
                                .set_status(QString::from(format!("Error interno: {error}")));
                        }
                    }
                })
                .ok();
        });
    }
}

fn apply_tracks(mut object: Pin<&mut qobject::CoreBridge>, tracks: Vec<Track>) {
    object.as_mut().set_connected(true);
    object.as_mut().set_count(tracks.len() as i32);

    let last = tracks.first().map(Track::display_name).unwrap_or_default();
    object.as_mut().set_last(QString::from(last));
    object
        .as_mut()
        .set_status(QString::from(format!("{} pistas cargadas", tracks.len())));
}

fn default_endpoint() -> String {
    std::env::var("DOPAMINE_SERVER_URL").unwrap_or_else(|_| String::from("http://127.0.0.1:8000"))
}

fn run_blocking<F>(future: F) -> Result<F::Output, String>
where
    F: Future,
{
    let runtime = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .map_err(|error| error.to_string())?;
    Ok(runtime.block_on(future))
}
