use crate::state::Track;
use std::path::Path;
use walkdir::WalkDir;

const SUPPORTED: &[&str] = &["mp3", "ogg", "flac", "wav", "m4a"];

pub fn scan(folder: &Path) -> Vec<Track> {
    WalkDir::new(folder)
        .into_iter()
        .filter_map(|e| e.ok())
        .filter(|e| {
            e.path()
             .extension()
             .and_then(|ext| ext.to_str())
             .map(|ext| SUPPORTED.contains(&ext.to_lowercase().as_str()))
             .unwrap_or(false)
        })
        .map(|e| Track {
            name: e.file_name().to_string_lossy().into_owned(),
            path: e.path().to_path_buf(),
        })
        .collect()
}
