/*
 * Copyright (C) 2020 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <glibmm/main.h>

#include "ardour/plugin_insert.h"
#include "ardour/vst3_plugin.h"

#include "gtkmm2ext/gui_thread.h"

#include "vst3_hwnd_plugin_ui.h"

#include <gdk/gdkwin32.h>

using namespace ARDOUR;
using namespace Steinberg;


VST3HWNDPluginUI::VST3HWNDPluginUI (boost::shared_ptr<PluginInsert> pi, boost::shared_ptr<VST3Plugin> vst3)
	: PlugUIBase (pi)
	, _pi (pi)
	, _vst3 (vst3)
	, _req_width (0)
	, _req_height (0)
{
	_ardour_buttons_box.set_spacing (6);
	_ardour_buttons_box.set_border_width (6);
	_ardour_buttons_box.pack_end (focus_button, false, false);
	_ardour_buttons_box.pack_end (bypass_button, false, false, 4);

	/* TODO Presets */

	_ardour_buttons_box.pack_end (pin_management_button, false, false);
	_ardour_buttons_box.pack_start (latency_button, false, false, 4);

	//_vst3->OnResizeView.connect (_resize_connection, invalidator (*this), boost::bind (&VST3HWNDPluginUI::resize_callback, this, _1, _2), gui_context());

	/* TODO register window class, implement wndproc etc */

	pack_start (_ardour_buttons_box, false, false);
	pack_start (_gui_widget, true, true);

	_gui_widget.signal_realize().connect (mem_fun (this, &VST3HWNDPluginUI::view_realized));
	_gui_widget.signal_size_request ().connect (mem_fun (this, &VST3HWNDPluginUI::view_size_request));
	_gui_widget.signal_size_allocate ().connect (mem_fun (this, &VST3HWNDPluginUI::view_size_allocate));

	_ardour_buttons_box.show_all ();
	_gui_widget.show ();
}

VST3HWNDPluginUI::~VST3HWNDPluginUI ()
{
	IPlugView* view = _vst3->view ();
	if (view) {
		view->removed ();
	}
}

gint
VST3HWNDPluginUI::get_preferred_height ()
{
	IPlugView* view = _vst3->view ();
	ViewRect rect;
	if (view && view->getSize (&rect) == kResultOk){
		return rect.bottom - rect.top;
	}
	return 0;
}

gint
VST3HWNDPluginUI::get_preferred_width ()
{
	IPlugView* view = _vst3->view ();
	ViewRect rect;
	if (view && view->getSize (&rect) == kResultOk){
		return rect.right - rect.left;
	}
	return 0;
}

bool
VST3HWNDPluginUI::resizable ()
{
	IPlugView* view = _vst3->view ();
	return view && view->canResize () == kResultTrue;
}

void
VST3HWNDPluginUI::view_realized ()
{
	IPlugView* view = _vst3->view ();
	HWND hwnd = (HWND) gdk_win32_drawable_get_handle (GTK_WIDGET(_gui_widget.gobj())->window);
	// SetWindowLongPtr (hwnd, GWLP_USERDATA, (__int3264) (LONG_PTR)this);
	if (kResultOk != view->attached (reinterpret_cast<void*> (hwnd), "HWND")) {
		assert (0);
	}

	ViewRect rect;
	if (view->getSize (&rect) == kResultOk) {
		_req_width  = rect.right - rect.left;
		_req_height = rect.bottom - rect.top;
	}
}

void
VST3HWNDPluginUI::view_size_request (GtkRequisition* requisition)
{
	requisition->width  = _req_width;
	requisition->height = _req_height;
}

void
VST3HWNDPluginUI::view_size_allocate (Gtk::Allocation& allocation)
{
	IPlugView* view = _vst3->view ();
	if (!view) {
		return;
	}
	ViewRect rect;
	if (view->getSize (&rect) == kResultOk) {
		rect.right = rect.left + allocation.get_width ();
		rect.bottom = rect.top + allocation.get_height ();
#if 0
		if (view->checkSizeConstraint (&rect) != kResultTrue) {
			view->getSize (&rect);
		}
		allocation.set_width (rect.right - rect.left);
		allocation.set_height (rect.bottom - rect.top);
#endif
		view->onSize (&rect);
	}
}

void
VST3HWNDPluginUI::resize_callback (int width, int height)
{
	//printf ("VST3HWNDPluginUI::resize_callback %d x %d\n", width, height);
#if 0
	HWND hwnd = gdk_win32_drawable_get_handle (_gui_widget.window);
	WINDOWINFO windowInfo;
	GetWindowInfo (hwnd, &windowInfo);
	RECT clientRect {};
	clientRect.right = newSize.width;
	clientRect.bottom = newSize.height;
	AdjustWindowRectEx (&clientRect, windowInfo.dwStyle, false, windowInfo.dwExStyle);
	SetWindowPos (hwnd, HWND_TOP, 0, 0, clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top, SWP_NOMOVE | SWP_NOCOPYBITS | SWP_NOACTIVATE);
#endif
}

bool
VST3HWNDPluginUI::start_updating (GdkEventAny*)
{
	return false;
}

bool
VST3HWNDPluginUI::stop_updating (GdkEventAny*)
{
	return false;
}

int
VST3HWNDPluginUI::package (Gtk::Window&)
{
	return 0;
}

bool
VST3HWNDPluginUI::on_window_show (const std::string& /*title*/)
{
	IPlugView* view = _vst3->view ();
	if (!view) {
		return false;
	}

	gtk_widget_realize (GTK_WIDGET(_gui_widget.gobj()));
	_gui_widget.show ();
	return true;
}

void
VST3HWNDPluginUI::on_window_hide ()
{
	_gui_widget.hide ();
}

void
VST3HWNDPluginUI::grab_focus ()
{
#if 0
	IPlugView* view = _vst3->view ();
	if (view) {
		view->onFocus (true);
	}
#endif
}
