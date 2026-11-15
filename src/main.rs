// Location: src/main.rs
// Function: Binary entry point enforcing single-instance dopamine:// URI
// handoff and launching the frameless transparent eframe/egui desktop shell.
// All Rights Reserved isgaar

use dopamine_desktop::app::DopamineApp;
use dopamine_desktop::core::single_instance::SingleInstanceGuard;

const APP_ICON_PNG: &[u8] = include_bytes!("shared/assets/app-icon.png");

fn main() -> eframe::Result<()> {
    let initial_uri = std::env::args()
        .skip(1)
        .find(|arg| arg.starts_with("dopamine://"));

    let single_instance = match SingleInstanceGuard::try_acquire(initial_uri.as_deref()) {
        Ok(guard) => Some(guard),
        Err(()) => {
            // An existing instance received our URI or raise notification.
            return Ok(());
        }
    };

    let mut viewport = egui::ViewportBuilder::default()
        .with_title("Dopamine")
        .with_app_id("com.isgaar.dopamine")
        .with_inner_size([1240.0, 800.0])
        .with_min_inner_size([840.0, 560.0])
        .with_decorations(false)
        .with_transparent(true);

    if let Ok(icon) = eframe::icon_data::from_png_bytes(APP_ICON_PNG) {
        viewport = viewport.with_icon(std::sync::Arc::new(icon));
    }

    let options = eframe::NativeOptions {
        viewport,
        ..Default::default()
    };

    eframe::run_native(
        "Dopamine",
        options,
        Box::new(move |cc| Ok(Box::new(DopamineApp::new(cc, single_instance, initial_uri)))),
    )
}

