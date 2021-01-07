#ifndef TVTORRENT_MACROS_H
#define TVTORRENT_MACROS_H

#include "glibmm/ustring.h"
#include <sigc++/functors/mem_fun.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/adaptors/hide.h>
#include <gtkmm/widget.h>

#define ON_CLICK(ev) signal_clicked().connect(sigc::mem_fun(*this, ev)) 
#define ON_BUTTON_PRESSED(ev) signal_button_press_event().connect(sigc::mem_fun(*this, ev))
#define ON_BUTTON_PRESSED_BIND(ev, types...) signal_button_press_event().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ON_CLICK_BIND(ev, types...)  signal_clicked().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ON_HIDE_BIND(ev, types...) signal_hide().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ON_HIDE(ev) signal_hide().connect(sigc::mem_fun(*this, ev))
#define HIDE_DIALOG_ON_CLOSE(dialog) signal_response().connect(sigc::hide(sigc::mem_fun(*(dialog), &Gtk::Widget::hide)))
#define ON_RESPONSE(ev) signal_response().connect(sigc::mem_fun(*this, ev))
#define ON_DISPATCH(ev) connect(sigc::mem_fun(*this, ev))
#define ON_DISPATCH_BIND(ev, types...) connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ADD_ACTION(name, ev) add_action(name, sigc::mem_fun(*this, ev))
#define ON_ACTIVATE(ev) signal_activate().connect(sigc::mem_fun(*this, ev))
#define ON_ACTIVATE_BIND(ev, types...) signal_activate().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ON_FOCUS_BIND(ev, types...) signal_focus().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ON_CHANGE(ev) signal_changed().connect(sigc::mem_fun(*this, ev))
#define ON_CHANGE_BIND(ev, types...) signal_changed().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ON_ROW_ACTIVATED(ev) signal_row_activated().connect(sigc::mem_fun(*this, ev))
#define ON_ROW_SELECTED(ev) signal_row_selected().connect(sigc::mem_fun(*this, ev))

#endif
