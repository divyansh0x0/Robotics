use crate::state::{Cmd, UiState};
use egui::Ui;
use tokio::sync::mpsc::Sender;

pub fn show(ui: &mut Ui, state: &mut UiState, cmd_tx: &Sender<Cmd>) {
    ui.horizontal(|ui| {
        // 1. LEFT CONTROLS
        if state.playing {
            if ui.button("⏸").clicked() {
                let _ = cmd_tx.try_send(Cmd::Pause);
                state.playing = false; // Optimistic update
            }
        } else {
            if ui.button("▶").clicked() {
                let _ = cmd_tx.try_send(Cmd::Resume);
                state.playing = true; // Optimistic update
            }
        }
        if ui.button("⏹").clicked() {
            let _ = cmd_tx.try_send(Cmd::Stop);
        }

        ui.add_space(10.0);

        let format_time = |ms: u64| {
            let secs = ms / 1000;
            let m = secs / 60;
            let s = secs % 60;
            format!("{:02}:{:02}", m, s)
        };

        ui.label(format_time(state.playback_pos_ms));
        ui.add_space(5.0);

        // 2. RIGHT SIDE CONTROLS
        // We evaluate these right-to-left so egui can calculate remaining space.
        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
            if let Some(idx) = state.current_track {
                if let Some(t) = state.tracks.get(idx) {
                    ui.label(egui::RichText::new(&t.name).strong());
                    ui.add_space(15.0);
                }
            }

            ui.label(format_time(state.playback_dur_ms));
            ui.add_space(5.0);

            // 3. CENTER / RESPONSIVE SLIDER
            // We revert to left-to-right for drawing the slider, and make it fill up all remaining available width.
            ui.with_layout(egui::Layout::left_to_right(egui::Align::Center), |ui| {
                let mut pos = state.playback_pos_ms as f64;
                let max_pos = state.playback_dur_ms as f64;
                
                let slider = egui::Slider::new(&mut pos, 0.0..=max_pos.max(100.0))
                    .show_value(false)
                    .trailing_fill(true);
                    
                // Consume all available horizontal space that we haven't given to right-elements
                let response = ui.add_sized(ui.available_size(), slider);
                
                if response.drag_stopped() || (response.changed() && ui.input(|i| i.pointer.any_click())) {
                    let _ = cmd_tx.try_send(Cmd::Seek(pos as u64));
                    state.playback_pos_ms = pos as u64; 
                }
            });
        });
    });
}
