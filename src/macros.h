#ifndef TVTORRENT_MACROS_H
#define TVTORRENT_MACROS_H

#include "glibmm/ustring.h"
#include <sigc++/functors/mem_fun.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/adaptors/hide.h>
#include <gtkmm/widget.h>

#define ON_CLICK(ev) signal_clicked().connect(sigc::mem_fun(*this, ev)) 
#define ON_BUTTON_PRESSED(ev) signal_button_press_event().connect(sigc::mem_fun(*this, ev))
#define ON_CLICK_BIND(ev, types...)  signal_clicked().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define ON_HIDE_BIND(ev, types...) signal_hide().connect(sigc::bind<types>(sigc::mem_fun(*this, ev)
#define HIDE_DIALOG_ON_CLOSE(dialog) signal_response().connect(sigc::hide(sigc::mem_fun(*dialog, &Gtk::Widget::hide)))
#define ON_RESPONSE(ev) signal_response().connect(sigc::mem_fun(*this, ev))
#define ON_DISPATCH(ev) connect(sigc::mem_fun(*this, ev))
#define ADD_ACTION(name, ev) add_action(name, sigc::mem_fun(*this, ev))

#endif
