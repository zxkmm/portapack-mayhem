/*
 * copyleft Elliot Alderson from F society
 * copyleft Darlene Alderson from F society
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_playlist_editor.hpp"
#include "ui_navigation.hpp"
#include "ui_external_items_menu_loader.hpp"

#include "file.hpp"
#include "ui_fileman.hpp"
#include "file_path.hpp"
#include "string_format.hpp"

namespace fs = std::filesystem;

#include "string_format.hpp"

#include "file_reader.hpp"
#include "usb_serial_asyncmsg.hpp"

using namespace portapack;

namespace ui::external_app::playlist_editor {

/*********menu**********/
PlaylistEditorView::PlaylistEditorView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &menu_view,
                  &text_app_info,
                  &button_open_playlist,
                  &button_edit,
                  &button_insert,
                  &button_save_playlist});

    menu_view.set_parent_rect({0, 2 * 8, screen_width, 24 * 8});

    menu_view.on_highlight = [this]() {
        text_app_info.set("Select an option:");
    };

    button_open_playlist.on_select = [this](Button&) {
        open_file();
    };

    button_edit.on_select = [this](Button&) {
        on_edit_item();
    };

    button_insert.on_select = [this](Button&) {
        on_insert_item();
    };

    button_save_playlist.on_select = [this](Button&) {
        save_ppl();
    };
}

void PlaylistEditorView::focus() {
    menu_view.focus();
}

void PlaylistEditorView::open_file() {
    auto open_view = nav_.push<FileLoadView>(".PPL");
    open_view->push_dir(playlist_dir);
    open_view->on_changed = [this](fs::path new_file_path) {
        current_ppl_path = new_file_path;
        on_file_changed(new_file_path);
    };
}

void PlaylistEditorView::on_file_changed(const fs::path& new_file_path) {
    File playlist_file;
    auto error = playlist_file.open(new_file_path.string());

    if (error) return;

    menu_view.clear();
    auto reader = FileLineReader(playlist_file);

    for (const auto& line : reader) {
        playlist.push_back(line);
    }

    for (const auto& line : playlist){
        //remove empty lines
        if (line == "\n" || line == "\r\n" || line == "\r") {
            playlist.erase(std::remove(playlist.begin(), playlist.end(), line), playlist.end());
        }

    }

    refresh_menu_view();
}

void PlaylistEditorView::refresh_menu_view(){

    menu_view.clear();

    for (const auto& line : playlist) {
        if (line.length() == 0 || line[0] == '#') {
            menu_view.add_item({line,
                                ui::Color::grey(),
                                &bitmap_icon_notepad,
                                [this](KeyEvent) {
                                    button_insert.focus();
                                }});
        } else {
            const auto filename = line.substr(line.find_last_of('/') + 1, line.find(',') - line.find_last_of('/') - 1);

            menu_view.add_item({filename,
                                ui::Color::white(),
                                &bitmap_icon_cwgen,
                                [this](KeyEvent) {
                                    button_edit.focus();
                                }});
        }
    }
}

// void PlaylistEditorView::on_edit_current_index() {
//     auto index = menu_view.highlighted_index();
//     const auto line = playlist[index];
//     if (index >= playlist.size()) return;
//     if (line[0] == '#') {
//         // edit comment
//     } else {
//         // edit item
//     }
// }

void PlaylistEditorView::on_edit_item() {
    portapack::async_tx_enabled = true;
    auto edit_view = nav_.push<PlaylistItemEditView>(
        playlist[menu_view.highlighted_index()]);

    edit_view->on_save = [this](std::string new_item) {
        UsbSerialAsyncmsg::asyncmsg(new_item);
        playlist[menu_view.highlighted_index()] = new_item;
        refresh_interface();
    };

    edit_view->on_delete = [this]() {
        playlist.erase(playlist.begin() + menu_view.highlighted_index());
        refresh_interface();
    };
}

void PlaylistEditorView::on_insert_item() {
    portapack::async_tx_enabled = true;
    auto edit_view = nav_.push<PlaylistItemEditView>(
        playlist[menu_view.highlighted_index()]);

    edit_view->on_save = [this](std::string new_item) {
        UsbSerialAsyncmsg::asyncmsg(new_item);
        playlist.insert(playlist.begin() + menu_view.highlighted_index() + 1, new_item);
        refresh_interface();
    };

    edit_view->on_delete = [this]() {
        playlist.erase(playlist.begin() + menu_view.highlighted_index());
        refresh_interface();
    };
}

void PlaylistEditorView::refresh_interface() {
    const auto previous_index = menu_view.highlighted_index();
    refresh_menu_view();
    set_dirty();
    menu_view.set_highlighted(previous_index);
}

void PlaylistEditorView::save_ppl() {
    if (current_ppl_path.empty() || playlist.empty()) {
        nav_.display_modal("Err", "No playlist file loaded");
        return;
    }

    File playlist_file;
    auto error = playlist_file.open(current_ppl_path.string(), false, false);

    if (error) {
        nav_.display_modal("Err", "open err");
        return;
    }

    //clear file
    playlist_file.seek(0);
    playlist_file.truncate();

    //write new data
    for (const auto& entry : playlist) {
        playlist_file.write_line(entry);
    }

    playlist_file.seek(0);
    for (const auto& entry : playlist) {
        if(entry == "\n" || entry == "\r\n" || entry == "\r" || entry.length() == 0) {
            playlist_file.write_line(entry);
        }
    }


    nav_.display_modal("Save", "Saved playlist\n" + current_ppl_path.string());
}


/*********edit**********/

PlaylistItemEditView::PlaylistItemEditView(
    NavigationView& nav,
    std::string item)
    : nav_{nav},
      original_item_{item} {

    add_children({
        &labels,
        &field_path,
        &field_delay, 
        &button_browse,
        &button_delete,
        &button_save
    });

    button_browse.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".C16");
        open_view->on_changed = [this](fs::path path) {
            field_path.set_text(path.string());
            path_ = path.string();
        };
    };

    button_delete.on_select = [this](Button&) {
        if (on_delete) on_delete();
        nav_.pop();
    };

    button_save.on_select = [this](Button&) {
        if (on_save) on_save(build_item());
        nav_.pop(); 
    };

    parse_item(item);
    refresh_ui();
}

void PlaylistItemEditView::focus() {
    button_save.focus();
}

void PlaylistItemEditView::refresh_ui() {
    field_path.set_text(path_);
    field_delay.set_value(delay_);
}


void PlaylistItemEditView::parse_item(std::string item) {
    // Parse format: path,delay
    auto parts = split_string(item, ',');
    if (parts.size() >= 1) {
        path_ = std::string{parts[0]};
    }
    if (parts.size() >= 2) {
        delay_ = atoi(std::string{parts[1]}.c_str());
    }
}

std::string PlaylistItemEditView::build_item() const {

    const auto v = path_ + "," + to_string_dec_uint(field_delay.value());
    UsbSerialAsyncmsg::asyncmsg(v);
    return path_ + "," + to_string_dec_uint(field_delay.value()); 
}

}  // namespace ui::external_app::playlist_editor