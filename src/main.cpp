#include "giomm/application.h"
#include "main_window.h"
#include "resource_manager.h"
#include <gtkmm/application.h>
#include <iostream>

namespace
{
int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
                    Glib::RefPtr<Gtk::Application>& app)
{
  int argc = 0;
  char** argv = command_line->get_arguments(argc);

//   for (int i = 0; i < argc; ++i)
//     std::cout << "argv[" << i << "] = " << argv[i] << std::endl;

  app->activate(); // Without activate() the window won't be shown.
  return EXIT_SUCCESS;
}

} // anonymous namespace

int main(int argc, char *argv[])
{

	auto app = Gtk::Application::create("com.sheepkill15.tvtorrent", Gio::APPLICATION_HANDLES_COMMAND_LINE | Gio::APPLICATION_HANDLES_OPEN);

	ResourceManager::init();

	TTMainWindow main_window;

	if(argc == 2) {
		main_window.external_torrent(argv[1]);
	}

	app->signal_command_line().connect(sigc::bind(sigc::ptr_fun(&on_command_line), app), false);

	return app->run(main_window);
	
}
