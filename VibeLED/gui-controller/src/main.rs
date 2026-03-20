mod audio;
mod config;
mod esp;
mod gui;
mod state;
mod worker;

use config::Config;
use gui::app::ControllerApp;
use state::{AppEvent, Cmd};

fn main() {
    let config = Config::load();

    let (cmd_tx, cmd_rx) = tokio::sync::mpsc::channel::<Cmd>(64);
    let (event_tx, event_rx) = tokio::sync::mpsc::channel::<AppEvent>(64);

    // Spawn the worker on its own OS thread because rodio::OutputStream is not `Send`
    // We create a single-threaded local Tokio runtime for it.
    let worker_config = config.clone();
    std::thread::spawn(move || {
        let rt = tokio::runtime::Builder::new_current_thread()
            .enable_all()
            .build()
            .unwrap();
        rt.block_on(worker::run(cmd_rx, event_tx, worker_config));
    });

    // eframe takes the main thread
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title("ESP Controller")
            .with_inner_size([960.0, 640.0]),
        ..Default::default()
    };

    eframe::run_native(
        "ESP Controller",
        options,
        Box::new(|_cc| Box::new(ControllerApp::new(cmd_tx, event_rx, config))),
    )
    .unwrap();
}
