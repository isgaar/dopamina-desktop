// Location: src/core/global_menu.rs
// Function: Native KDE Plasma Global Menu integration implementing
// com.canonical.dbusmenu over Session D-Bus, KWin Wayland
// org_kde_kwin_appmenu_manager registration, X11 AppMenu.Registrar fallback,
// and shared in-app menu model.
// All Rights Reserved isgaar

use std::collections::HashMap;
use std::sync::mpsc::{self, Receiver, Sender};
use std::sync::{Arc, Mutex};

use egui::Context;
use raw_window_handle::{HasDisplayHandle, HasWindowHandle, RawDisplayHandle, RawWindowHandle};
use wayland_backend::sys::client::Backend as ForeignWaylandBackend;
use wayland_client::backend::ObjectId;
use wayland_client::globals::{registry_queue_init, GlobalListContents};
use wayland_client::protocol::wl_registry::WlRegistry;
use wayland_client::protocol::wl_surface::WlSurface;
use wayland_client::{Connection as WlConnection, Dispatch, Proxy, QueueHandle};
use wayland_protocols_plasma::appmenu::client::{
    org_kde_kwin_appmenu::OrgKdeKwinAppmenu,
    org_kde_kwin_appmenu_manager::OrgKdeKwinAppmenuManager,
};
use zbus::object_server::SignalEmitter;
use zbus::zvariant::{OwnedValue, StructureBuilder, Value};
use zbus::{interface, Connection as DbusConnection};

use crate::shared::i18n::StringCatalog;

pub const MENU_OBJECT_PATH: &str = "/MenuBar";

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum GlobalMenuAction {
    OpenView(String),
    PlayPause,
    NextTrack,
    PreviousTrack,
    ToggleShuffle,
    ToggleRepeat,
    ToggleLyrics,
    ToggleVisualizer,
    NewPlaylist,
    RefreshLibrary,
    OpenEqualizer,
    OpenSleepTimer,
    OpenSession,
    OpenSettings,
    OpenAbout,
    Quit,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GlobalMenuSnapshot {
    pub language: String,
    pub is_playing: bool,
    pub is_shuffle: bool,
    pub is_repeat: bool,
    pub show_lyrics: bool,
    pub show_visualizer: bool,
    pub is_logged_in: bool,
}

impl Default for GlobalMenuSnapshot {
    fn default() -> Self {
        Self {
            language: String::from("system"),
            is_playing: false,
            is_shuffle: false,
            is_repeat: false,
            show_lyrics: false,
            show_visualizer: false,
            is_logged_in: false,
        }
    }
}

#[derive(Debug, Clone)]
pub struct MenuEntry {
    pub id: i32,
    pub label: String,
    pub is_separator: bool,
    pub toggle_checked: Option<bool>,
    pub action: Option<GlobalMenuAction>,
    pub children: Vec<MenuEntry>,
}

impl MenuEntry {
    fn submenu(id: i32, label: &str, children: Vec<MenuEntry>) -> Self {
        Self {
            id,
            label: label.to_string(),
            is_separator: false,
            toggle_checked: None,
            action: None,
            children,
        }
    }

    fn item(id: i32, label: &str, action: GlobalMenuAction) -> Self {
        Self {
            id,
            label: label.to_string(),
            is_separator: false,
            toggle_checked: None,
            action: Some(action),
            children: Vec::new(),
        }
    }

    fn check(id: i32, label: &str, checked: bool, action: GlobalMenuAction) -> Self {
        Self {
            id,
            label: label.to_string(),
            is_separator: false,
            toggle_checked: Some(checked),
            action: Some(action),
            children: Vec::new(),
        }
    }

    fn separator(id: i32) -> Self {
        Self {
            id,
            label: String::new(),
            is_separator: true,
            toggle_checked: None,
            action: None,
            children: Vec::new(),
        }
    }
}

pub fn build_menu_entries(snapshot: &GlobalMenuSnapshot) -> Vec<MenuEntry> {
    let strings = StringCatalog::new(&snapshot.language);

    let play_label = if snapshot.is_playing {
        strings.get("menu_pause")
    } else {
        strings.get("menu_play")
    };

    vec![
        MenuEntry::submenu(
            100,
            strings.get("menu_file"),
            vec![
                MenuEntry::item(
                    101,
                    strings.get("menu_new_playlist"),
                    GlobalMenuAction::NewPlaylist,
                ),
                MenuEntry::item(
                    102,
                    strings.get("menu_refresh"),
                    GlobalMenuAction::RefreshLibrary,
                ),
                MenuEntry::separator(103),
                MenuEntry::item(
                    104,
                    strings.get("menu_session"),
                    GlobalMenuAction::OpenSession,
                ),
                MenuEntry::item(
                    105,
                    strings.get("menu_settings"),
                    GlobalMenuAction::OpenSettings,
                ),
                MenuEntry::separator(106),
                MenuEntry::item(107, strings.get("menu_exit"), GlobalMenuAction::Quit),
            ],
        ),
        MenuEntry::submenu(
            200,
            strings.get("menu_playback"),
            vec![
                MenuEntry::item(201, play_label, GlobalMenuAction::PlayPause),
                MenuEntry::item(
                    202,
                    strings.get("menu_previous"),
                    GlobalMenuAction::PreviousTrack,
                ),
                MenuEntry::item(203, strings.get("menu_next"), GlobalMenuAction::NextTrack),
                MenuEntry::separator(204),
                MenuEntry::check(
                    205,
                    strings.get("menu_shuffle"),
                    snapshot.is_shuffle,
                    GlobalMenuAction::ToggleShuffle,
                ),
                MenuEntry::check(
                    206,
                    strings.get("menu_repeat"),
                    snapshot.is_repeat,
                    GlobalMenuAction::ToggleRepeat,
                ),
            ],
        ),
        MenuEntry::submenu(
            300,
            strings.get("menu_view"),
            vec![
                MenuEntry::item(
                    301,
                    strings.get("menu_view_listen"),
                    GlobalMenuAction::OpenView(String::from("listen")),
                ),
                MenuEntry::item(
                    302,
                    strings.get("menu_view_library"),
                    GlobalMenuAction::OpenView(String::from("library")),
                ),
                MenuEntry::item(
                    303,
                    strings.get("menu_view_favorites"),
                    GlobalMenuAction::OpenView(String::from("favorites")),
                ),
                MenuEntry::item(
                    304,
                    strings.get("menu_view_playlists"),
                    GlobalMenuAction::OpenView(String::from("playlists")),
                ),
                MenuEntry::item(
                    305,
                    strings.get("menu_view_queue"),
                    GlobalMenuAction::OpenView(String::from("queue")),
                ),
                MenuEntry::separator(306),
                MenuEntry::check(
                    307,
                    strings.get("menu_lyrics"),
                    snapshot.show_lyrics,
                    GlobalMenuAction::ToggleLyrics,
                ),
                MenuEntry::check(
                    308,
                    strings.get("menu_visualizer"),
                    snapshot.show_visualizer,
                    GlobalMenuAction::ToggleVisualizer,
                ),
            ],
        ),
        MenuEntry::submenu(
            400,
            strings.get("menu_audio"),
            vec![
                MenuEntry::item(
                    401,
                    strings.get("menu_equalizer"),
                    GlobalMenuAction::OpenEqualizer,
                ),
                MenuEntry::item(
                    402,
                    strings.get("menu_sleep_timer"),
                    GlobalMenuAction::OpenSleepTimer,
                ),
            ],
        ),
        MenuEntry::submenu(
            500,
            strings.get("menu_help"),
            vec![MenuEntry::item(
                501,
                strings.get("menu_about"),
                GlobalMenuAction::OpenAbout,
            )],
        ),
    ]
}

struct SharedMenuState {
    revision: u32,
    entries: Vec<MenuEntry>,
}

struct DbusMenuObject {
    state: Arc<Mutex<SharedMenuState>>,
    action_tx: Sender<GlobalMenuAction>,
    egui_ctx: Context,
}

#[interface(name = "com.canonical.dbusmenu")]
impl DbusMenuObject {
    #[zbus(property)]
    fn version(&self) -> u32 {
        3
    }

    #[zbus(property)]
    fn text_direction(&self) -> String {
        String::from("ltr")
    }

    #[zbus(property)]
    fn status(&self) -> String {
        String::from("normal")
    }

    #[zbus(property)]
    fn icon_theme_path(&self) -> Vec<String> {
        Vec::new()
    }

    async fn get_layout(
        &self,
        parent_id: i32,
        recursion_depth: i32,
        property_names: Vec<String>,
    ) -> zbus::fdo::Result<(u32, (i32, HashMap<String, OwnedValue>, Vec<OwnedValue>))> {
        let guard = self
            .state
            .lock()
            .map_err(|_| zbus::fdo::Error::Failed(String::from("menu lock poisoned")))?;

        let root_children: Vec<OwnedValue> = if recursion_depth == 0 {
            Vec::new()
        } else {
            let next_depth = if recursion_depth < 0 {
                -1
            } else {
                recursion_depth - 1
            };
            if parent_id == 0 {
                guard
                    .entries
                    .iter()
                    .filter_map(|e| entry_to_owned_value(e, next_depth, &property_names))
                    .collect()
            } else if let Some(node) = find_entry(&guard.entries, parent_id) {
                node.children
                    .iter()
                    .filter_map(|e| entry_to_owned_value(e, next_depth, &property_names))
                    .collect()
            } else {
                Vec::new()
            }
        };

        let root_props = if parent_id == 0 {
            let mut m = HashMap::new();
            if should_include("children-display", &property_names) {
                if let Ok(v) = OwnedValue::try_from(Value::from("submenu")) {
                    m.insert(String::from("children-display"), v);
                }
            }
            m
        } else if let Some(node) = find_entry(&guard.entries, parent_id) {
            entry_props(node, &property_names)
        } else {
            HashMap::new()
        };

        Ok((guard.revision, (parent_id, root_props, root_children)))
    }

    async fn get_group_properties(
        &self,
        ids: Vec<i32>,
        property_names: Vec<String>,
    ) -> zbus::fdo::Result<Vec<(i32, HashMap<String, OwnedValue>)>> {
        let guard = self
            .state
            .lock()
            .map_err(|_| zbus::fdo::Error::Failed(String::from("menu lock poisoned")))?;
        let mut out = Vec::new();
        for id in ids {
            if let Some(node) = find_entry(&guard.entries, id) {
                out.push((id, entry_props(node, &property_names)));
            }
        }
        Ok(out)
    }

    async fn get_property(&self, id: i32, name: String) -> zbus::fdo::Result<OwnedValue> {
        let guard = self
            .state
            .lock()
            .map_err(|_| zbus::fdo::Error::Failed(String::from("menu lock poisoned")))?;
        let Some(node) = find_entry(&guard.entries, id) else {
            return Err(zbus::fdo::Error::Failed(format!("unknown menu id {id}")));
        };
        let props = entry_props(node, &[name.clone()]);
        props
            .into_iter()
            .next()
            .map(|(_, v)| v)
            .ok_or_else(|| zbus::fdo::Error::Failed(format!("missing property {name}")))
    }

    async fn event(
        &self,
        id: i32,
        event_id: String,
        _data: OwnedValue,
        _timestamp: u32,
    ) -> zbus::fdo::Result<()> {
        if event_id == "clicked" {
            let action = self
                .state
                .lock()
                .ok()
                .and_then(|g| find_entry(&g.entries, id).and_then(|e| e.action.clone()));
            if let Some(act) = action {
                let _ = self.action_tx.send(act);
                self.egui_ctx.request_repaint();
            }
        }
        Ok(())
    }

    async fn event_group(
        &self,
        events: Vec<(i32, String, OwnedValue, u32)>,
    ) -> zbus::fdo::Result<Vec<i32>> {
        for (id, event_id, _data, _ts) in events {
            if event_id == "clicked" {
                let action = self
                    .state
                    .lock()
                    .ok()
                    .and_then(|g| find_entry(&g.entries, id).and_then(|e| e.action.clone()));
                if let Some(act) = action {
                    let _ = self.action_tx.send(act);
                    self.egui_ctx.request_repaint();
                }
            }
        }
        Ok(Vec::new())
    }

    async fn about_to_show(&self, _id: i32) -> bool {
        false
    }

    async fn about_to_show_group(&self, _ids: Vec<i32>) -> (Vec<i32>, Vec<i32>) {
        (Vec::new(), Vec::new())
    }

    #[zbus(signal)]
    async fn layout_updated(
        emitter: &SignalEmitter<'_>,
        revision: u32,
        parent: i32,
    ) -> zbus::Result<()>;
}

fn find_entry<'a>(entries: &'a [MenuEntry], id: i32) -> Option<&'a MenuEntry> {
    for entry in entries {
        if entry.id == id {
            return Some(entry);
        }
        if let Some(found) = find_entry(&entry.children, id) {
            return Some(found);
        }
    }
    None
}

fn should_include(name: &str, filter: &[String]) -> bool {
    filter.is_empty() || filter.iter().any(|p| p == name)
}

fn entry_props(entry: &MenuEntry, filter: &[String]) -> HashMap<String, OwnedValue> {
    let mut map = HashMap::new();
    let mut put = |k: &str, v: Value<'_>| {
        if should_include(k, filter) {
            if let Ok(ov) = OwnedValue::try_from(v) {
                map.insert(k.to_string(), ov);
            }
        }
    };

    if entry.is_separator {
        put("type", Value::from("separator"));
        put("visible", Value::from(true));
        return map;
    }

    put("label", Value::from(entry.label.as_str()));
    put("enabled", Value::from(true));
    put("visible", Value::from(true));

    if !entry.children.is_empty() {
        put("children-display", Value::from("submenu"));
    }
    if let Some(checked) = entry.toggle_checked {
        put("toggle-type", Value::from("checkmark"));
        put("toggle-state", Value::from(if checked { 1_i32 } else { 0_i32 }));
    }
    map
}

fn entry_to_owned_value(
    entry: &MenuEntry,
    depth: i32,
    filter: &[String],
) -> Option<OwnedValue> {
    let props = entry_props(entry, filter);
    let children: Vec<OwnedValue> = if depth == 0 || entry.children.is_empty() {
        Vec::new()
    } else {
        let next_depth = if depth < 0 { -1 } else { depth - 1 };
        entry
            .children
            .iter()
            .filter_map(|c| entry_to_owned_value(c, next_depth, filter))
            .collect()
    };

    let structure = StructureBuilder::new()
        .add_field(entry.id)
        .add_field(props)
        .add_field(children)
        .build()
        .ok()?;
    OwnedValue::try_from(Value::Structure(structure)).ok()
}

enum WindowBindingTarget {
    Wayland {
        display_ptr: usize,
        surface_ptr: usize,
    },
    X11(u32),
    None,
}

struct WaylandMenuBinding {
    _conn: WlConnection,
    _appmenu: OrgKdeKwinAppmenu,
}

struct WlBindState;

impl Dispatch<WlRegistry, GlobalListContents> for WlBindState {
    fn event(
        _state: &mut Self,
        _proxy: &WlRegistry,
        _event: wayland_client::protocol::wl_registry::Event,
        _data: &GlobalListContents,
        _conn: &WlConnection,
        _qhandle: &QueueHandle<Self>,
    ) {
    }
}

impl Dispatch<OrgKdeKwinAppmenuManager, ()> for WlBindState {
    fn event(
        _state: &mut Self,
        _proxy: &OrgKdeKwinAppmenuManager,
        _event: wayland_protocols_plasma::appmenu::client::org_kde_kwin_appmenu_manager::Event,
        _data: &(),
        _conn: &WlConnection,
        _qhandle: &QueueHandle<Self>,
    ) {
    }
}

impl Dispatch<OrgKdeKwinAppmenu, ()> for WlBindState {
    fn event(
        _state: &mut Self,
        _proxy: &OrgKdeKwinAppmenu,
        _event: wayland_protocols_plasma::appmenu::client::org_kde_kwin_appmenu::Event,
        _data: &(),
        _conn: &WlConnection,
        _qhandle: &QueueHandle<Self>,
    ) {
    }
}

pub struct GlobalMenuService {
    state: Arc<Mutex<SharedMenuState>>,
    snapshot: GlobalMenuSnapshot,
    action_rx: Receiver<GlobalMenuAction>,
    bus_name_rx: Receiver<String>,
    dbus_update_tx: Sender<u32>,
    target: WindowBindingTarget,
    bound: bool,
    wayland_binding: Option<WaylandMenuBinding>,
}

impl GlobalMenuService {
    pub fn start(cc: &eframe::CreationContext<'_>, snapshot: GlobalMenuSnapshot) -> Self {
        let entries = build_menu_entries(&snapshot);
        let state = Arc::new(Mutex::new(SharedMenuState {
            revision: 1,
            entries,
        }));
        let (action_tx, action_rx) = mpsc::channel();
        let (bus_name_tx, bus_name_rx) = mpsc::channel();
        let (dbus_update_tx, dbus_update_rx) = mpsc::channel();

        let target = extract_window_target(cc);
        let x11_window_id = match target {
            WindowBindingTarget::X11(xid) => Some(xid),
            _ => None,
        };

        let dbus_state = state.clone();
        let egui_ctx = cc.egui_ctx.clone();
        std::thread::Builder::new()
            .name(String::from("dopamine-dbusmenu"))
            .spawn(move || {
                let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                    .enable_all()
                    .build()
                else {
                    return;
                };
                rt.block_on(async move {
                    let menu_obj = DbusMenuObject {
                        state: dbus_state,
                        action_tx,
                        egui_ctx: egui_ctx.clone(),
                    };
                    let Ok(conn) = DbusConnection::session().await else {
                        return;
                    };
                    if conn
                        .object_server()
                        .at(MENU_OBJECT_PATH, menu_obj)
                        .await
                        .is_err()
                    {
                        return;
                    }
                    if let Some(unique) = conn.unique_name() {
                        let _ = bus_name_tx.send(unique.as_str().to_string());
                        egui_ctx.request_repaint();
                    }
                    if let Some(xid) = x11_window_id {
                        let path = zbus::zvariant::ObjectPath::from_static_str_unchecked(
                            MENU_OBJECT_PATH,
                        );
                        let _ = conn
                            .call_method(
                                Some("com.canonical.AppMenu.Registrar"),
                                "/com/canonical/AppMenu/Registrar",
                                Some("com.canonical.AppMenu.Registrar"),
                                "RegisterWindow",
                                &(xid, path),
                            )
                            .await;
                    }

                    loop {
                        while let Ok(rev) = dbus_update_rx.try_recv() {
                            if let Ok(iface) = conn
                                .object_server()
                                .interface::<_, DbusMenuObject>(MENU_OBJECT_PATH)
                                .await
                            {
                                let _ =
                                    DbusMenuObject::layout_updated(iface.signal_emitter(), rev, 0)
                                        .await;
                            }
                        }
                        tokio::time::sleep(std::time::Duration::from_millis(60)).await;
                    }
                });
            })
            .ok();

        Self {
            state,
            snapshot,
            action_rx,
            bus_name_rx,
            dbus_update_tx,
            target,
            bound: matches!(x11_window_id, Some(_)),
            wayland_binding: None,
        }
    }

    pub fn update_snapshot(&mut self, next: GlobalMenuSnapshot) {
        if self.snapshot == next {
            return;
        }
        self.snapshot = next;
        let entries = build_menu_entries(&self.snapshot);
        if let Ok(mut guard) = self.state.lock() {
            guard.revision = guard.revision.saturating_add(1);
            guard.entries = entries;
            let _ = self.dbus_update_tx.send(guard.revision);
        }
    }

    pub fn poll_actions(&mut self) -> Vec<GlobalMenuAction> {
        if !self.bound {
            if let Ok(bus_name) = self.bus_name_rx.try_recv() {
                self.bound = true;
                if let WindowBindingTarget::Wayland {
                    display_ptr,
                    surface_ptr,
                } = self.target
                {
                    self.wayland_binding =
                        bind_kde_wayland_appmenu(display_ptr, surface_ptr, &bus_name);
                }
            }
        }

        let mut actions = Vec::new();
        while let Ok(act) = self.action_rx.try_recv() {
            actions.push(act);
        }
        actions
    }

    pub fn entries(&self) -> Vec<MenuEntry> {
        build_menu_entries(&self.snapshot)
    }
}

fn extract_window_target(cc: &eframe::CreationContext<'_>) -> WindowBindingTarget {
    let Ok(display) = cc.display_handle() else {
        return WindowBindingTarget::None;
    };
    let Ok(window) = cc.window_handle() else {
        return WindowBindingTarget::None;
    };
    match (display.as_raw(), window.as_raw()) {
        (RawDisplayHandle::Wayland(d), RawWindowHandle::Wayland(w)) => {
            WindowBindingTarget::Wayland {
                display_ptr: d.display.as_ptr() as usize,
                surface_ptr: w.surface.as_ptr() as usize,
            }
        }
        (_, RawWindowHandle::Xlib(w)) => WindowBindingTarget::X11(w.window as u32),
        (_, RawWindowHandle::Xcb(w)) => WindowBindingTarget::X11(w.window.get()),
        _ => WindowBindingTarget::None,
    }
}

fn bind_kde_wayland_appmenu(
    display_ptr: usize,
    surface_ptr: usize,
    dbus_service_name: &str,
) -> Option<WaylandMenuBinding> {
    if display_ptr == 0 || surface_ptr == 0 {
        return None;
    }
    let backend = unsafe { ForeignWaylandBackend::from_foreign_display(display_ptr as *mut _) };
    let conn = WlConnection::from_backend(backend);
    let surface_id =
        unsafe { ObjectId::from_ptr(WlSurface::interface(), surface_ptr as *mut _) }.ok()?;
    let wl_surface = WlSurface::from_id(&conn, surface_id).ok()?;

    let (globals, mut queue) = registry_queue_init::<WlBindState>(&conn).ok()?;
    let qh = queue.handle();
    let manager: OrgKdeKwinAppmenuManager = globals.bind(&qh, 1..=2, ()).ok()?;
    let appmenu = manager.create(&wl_surface, &qh, ());
    appmenu.set_address(dbus_service_name.to_string(), MENU_OBJECT_PATH.to_string());
    let _ = queue.roundtrip(&mut WlBindState);
    let _ = conn.flush();

    Some(WaylandMenuBinding {
        _conn: conn,
        _appmenu: appmenu,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn builds_localized_global_menu_tree() {
        let mut snap = GlobalMenuSnapshot::default();
        snap.language = String::from("es");
        let es_entries = build_menu_entries(&snap);
        assert_eq!(es_entries.len(), 5);
        assert_eq!(es_entries[0].label, "Archivo");
        assert!(find_entry(&es_entries, 205).is_some());

        snap.language = String::from("en");
        let en_entries = build_menu_entries(&snap);
        assert_eq!(en_entries[0].label, "File");
    }
}

