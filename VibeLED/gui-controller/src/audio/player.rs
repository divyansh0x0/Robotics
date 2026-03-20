use rodio::{Decoder, OutputStream, OutputStreamHandle, Sink};
use std::fs::File;
use std::io::BufReader;
use std::path::Path;

pub struct Player {
    _stream: OutputStream,          // must be kept alive
    handle:  OutputStreamHandle,
    sink:    Option<Sink>,
}

impl Player {
    pub fn new() -> Result<Self, String> {
        let (_stream, handle) = OutputStream::try_default().map_err(|e| e.to_string())?;
        Ok(Self {
            _stream,
            handle,
            sink: None,
        })
    }

    pub fn play(&mut self, path: &Path) -> Result<(), String> {
        self.stop();
        let file   = BufReader::new(File::open(path).map_err(|e| e.to_string())?);
        let source = Decoder::new(file).map_err(|e| e.to_string())?;
        let sink   = Sink::try_new(&self.handle).map_err(|e| e.to_string())?;
        sink.append(source);
        sink.play();
        self.sink = Some(sink);
        Ok(())
    }

    pub fn pause(&self)  { if let Some(s) = &self.sink { s.pause(); } }
    pub fn resume(&self) { if let Some(s) = &self.sink { s.play();  } }
    pub fn stop(&mut self) { self.sink.take(); }

    pub fn is_paused(&self) -> bool { self.sink.as_ref().map(|s| s.is_paused()).unwrap_or(true) }
}
