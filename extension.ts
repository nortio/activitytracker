/* extension.ts
 *
 * Copyright 2024 nortio
 * 
 * SPDX-License-Identifier: MIT
 */

import GObject from 'gi://GObject';
import St from 'gi://St';
import GLib from 'gi://GLib';
import Meta from 'gi://Meta';

import { Extension, ExtensionMetadata, gettext as _ } from 'resource:///org/gnome/shell/extensions/extension.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';

import * as Main from 'resource:///org/gnome/shell/ui/main.js';

/* const Indicator = GObject.registerClass(
class Indicator extends PanelMenu.Button {
    _init() {
        
    }
}); */

import Gio from 'gi://Gio';



export default class ActivityTrackerExtension extends Extension {
    private _screenLocked: boolean;
    private _idle: boolean;
    private _indicator: PanelMenu.Button | undefined;
    private _sessionId: any | null;
    private _timeoutId: any | null;
    private _idleMonitor: Meta.IdleMonitor | undefined;
    private _logLocation: Gio.File;
    private _logFile: Gio.FileOutputStream | undefined;
    private _encoder: TextEncoder;


    constructor(metadata: ExtensionMetadata) {
        super(metadata)
        this._screenLocked = false;
        this._indicator = undefined;
        this._idle = false;

        this._encoder = new TextEncoder();
        this._logLocation = Gio.file_new_build_filenamev([GLib.get_home_dir(), ".local/share/activitytracker/log"]);
        Gio._promisify(Gio.OutputStream.prototype, 'write_bytes_async', 'write_bytes_finish');
    }

    _onSessionModeChange(session: { currentMode: string; parentMode: string; }) {
        if (session.currentMode === "user" || session.parentMode === "user") {
            this._screenLocked = false;
        } else {
            this._screenLocked = true;
        }
    }

    async write_to_log(name: string) {
        let utf = this._encoder.encode(name);
        let bytes = new GLib.Bytes(utf);
        let num = await this._logFile?.write_bytes_async(bytes, GLib.PRIORITY_DEFAULT, null) as unknown as number;

        console.log(`Written ${num} bytes`);
    }


    async enable() {
        this._onSessionModeChange(Main.sessionMode)
        if (!this._indicator) {
            this._indicator = new PanelMenu.Button(0.0, _('Activity Tracker Indicator'));
        }

        if (!this._logFile) {
            console.log(`Logging to ${this._logLocation.get_path()}`);
            this._logFile = this._logLocation.append_to(Gio.FileCreateFlags.NONE, null);

            //await this.write_to_log("hello world\n")
        }

        this._indicator.add_child(new St.Icon({
            icon_name: 'face-smile-symbolic',
            style_class: 'system-status-icon',
        }));

        let item = new PopupMenu.PopupMenuItem(_('Is inactive?'));
        item.connect('activate', () => {
            Main.notify("Inactive: ", this._screenLocked.toString());
        });

        let menu = this._indicator.menu as PopupMenu.PopupMenu<PopupMenu.PopupMenu.SignalMap>;
        menu.addMenuItem(item);

        Main.panel.addToStatusArea(this.uuid, this._indicator);

        this._sessionId = Main.sessionMode.connect('updated', this._onSessionModeChange.bind(this));

        this._timeoutId = GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 10, () => {
            //console.warn(`Inactive: ${this._screenLocked}`);
            //Main.notify("Tick", `Inactive: ${this._screenLocked}`);
            let entry = {
                timestamp: new Date().valueOf(),
                locked: this._screenLocked,
                idle: this._idle,
                name: "",
                pid: "",
                id: "",
                class: ""
            }

            if (!this._screenLocked) {
                let focused_windows = global.get_window_actors()
                    .map(a => a.meta_window)
                    .filter(w => w.has_focus())
                    .map(w => ({ pid: w.get_pid(), name: w.get_title(), id: w.get_sandboxed_app_id() ? w.get_sandboxed_app_id() : w.get_gtk_application_id(), class: w.get_wm_class() }))
                console.log(focused_windows)
                if (focused_windows.length > 0) {
                    let focused_window = focused_windows[0];
                    entry.class = focused_window.class ? focused_window.class : "";
                    entry.id = focused_window.id ? focused_window.id : "";
                    entry.name = focused_window.name ? focused_window.name : "";
                    entry.pid = focused_window.pid ? focused_window.pid.toString() : "";
                }
            }

            this.write_to_log(`${entry.timestamp},${entry.locked},${entry.idle},${entry.name.replace(",", "\\,")},${entry.pid},${entry.id},${entry.class}\n`)

            return GLib.SOURCE_CONTINUE
        })

        this._idleMonitor = global.backend.get_core_idle_monitor();
        this._idleMonitor.add_idle_watch(1000 * 120, () => {
            console.log("User is idle");
            //Main.notify("Idle", "User idle")
            this._idle = true;
            this._idleMonitor!.add_user_active_watch(() => {
                console.log("User is back")
                this._idle = false;
            })
        })



        console.error("HELLO WORLD")
    }

    disable() {
        if (this._indicator) {
            this._indicator.destroy();
            this._indicator = undefined;
        }

        if (this._timeoutId) {
            GLib.Source.remove(this._timeoutId);
            this._timeoutId = null;
        }

        if (this._sessionId) {
            Main.sessionMode.disconnect(this._sessionId);
            this._sessionId = null;
        }


    }
}
