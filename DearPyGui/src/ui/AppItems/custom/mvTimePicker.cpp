#include "mvTimePicker.h"
#include <implot.h>
#include <implot_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "mvContext.h"
#include "mvItemRegistry.h"
#include "mvPythonExceptions.h"
#include "AppItems/fonts/mvFont.h"
#include "AppItems/themes/mvTheme.h"
#include "AppItems/containers/mvDragPayload.h"

namespace Marvel {

	mvTimePicker::mvTimePicker(mvUUID uuid)
		: 
		mvAppItem(uuid)
	{
	}

	void mvTimePicker::applySpecificTemplate(mvAppItem* item)
	{
		auto titem = static_cast<mvTimePicker*>(item);
		if (_source != 0)
                { 
                    _value = titem->_value;
		    _imvalue = titem->_imvalue;
                }
		_hour24 = titem->_hour24;
	}

	void mvTimePicker::draw(ImDrawList* drawlist, float x, float y)
	{

		//-----------------------------------------------------------------------------
		// pre draw
		//-----------------------------------------------------------------------------

		// show/hide
		if (!_show)
			return;

		// focusing
		if (_focusNextFrame)
		{
			ImGui::SetKeyboardFocusHere();
			_focusNextFrame = false;
		}

		// cache old cursor position
		ImVec2 previousCursorPos = ImGui::GetCursorPos();

		// set cursor position if user set
		if (_dirtyPos)
			ImGui::SetCursorPos(_state.pos);

		// update widget's position state
		_state.pos = { ImGui::GetCursorPosX(), ImGui::GetCursorPosY() };

		// set item width
		if (_width != 0)
			ImGui::SetNextItemWidth((float)_width);

		// set indent
		if (_indent > 0.0f)
			ImGui::Indent(_indent);

		// push font if a font object is attached
		if (_font)
		{
			ImFont* fontptr = static_cast<mvFont*>(_font.get())->getFontPtr();
			ImGui::PushFont(fontptr);
		}

		// themes
		apply_local_theming(this);

		//-----------------------------------------------------------------------------
		// draw
		//-----------------------------------------------------------------------------
		{

			ScopedID id(_uuid);

			ImPlot::GetStyle().Use24HourClock = _hour24;

			if (ImPlot::ShowTimePicker(_internalLabel.c_str(), _imvalue.get()))
			{
				ImPlot::GetGmtTime(*_imvalue, _value.get());
				{
					auto value = *_value;
					mvSubmitCallback([=]() {
						if(_alias.empty())
							mvAddCallback(getCallback(false), _uuid, ToPyTime(value), _user_data);
						else
							mvAddCallback(getCallback(false), _alias, ToPyTime(value), _user_data);
						});
				}
			}
		}

		//-----------------------------------------------------------------------------
		// update state
		//-----------------------------------------------------------------------------
		UpdateAppItemState(_state);

		//-----------------------------------------------------------------------------
		// post draw
		//-----------------------------------------------------------------------------

		// set cursor position to cached position
		if (_dirtyPos)
			ImGui::SetCursorPos(previousCursorPos);

		if (_indent > 0.0f)
			ImGui::Unindent(_indent);

		// pop font off stack
		if (_font)
			ImGui::PopFont();

		// handle popping themes
		cleanup_local_theming(this);

		if (_handlerRegistry)
			_handlerRegistry->customAction(&_state);

		// handle drag & drop payloads
		for (auto& item : _children[3])
			item->draw(nullptr, ImGui::GetCursorPosX(), ImGui::GetCursorPosY());

		// handle drag & drop if used
		if (_dropCallback)
		{
			ScopedID id(_uuid);
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(_payloadType.c_str()))
				{
					auto payloadActual = static_cast<const mvDragPayload*>(payload->Data);
					if (_alias.empty())
						mvAddCallback(_dropCallback,_uuid, payloadActual->getDragData(), nullptr);
					else
						mvAddCallback(_dropCallback,_alias, payloadActual->getDragData(), nullptr);
				}

				ImGui::EndDragDropTarget();
			}
		}

	}

	void mvTimePicker::handleSpecificKeywordArgs(PyObject* dict)
	{
		if (dict == nullptr)
			return;
		 
		if (PyObject* item = PyDict_GetItemString(dict, "hour24")) _hour24 = ToBool(item);
	}

	void mvTimePicker::getSpecificConfiguration(PyObject* dict)
	{
		if (dict == nullptr)
			return;
		 
		PyDict_SetItemString(dict, "hour24", mvPyObject(ToPyBool(_hour24)));
	}

	PyObject* mvTimePicker::getPyValue()
	{
		return ToPyTime(*_value);
	}

	void mvTimePicker::setPyValue(PyObject* value)
	{

		if (value)
			*_value = ToTime(value);
		else
			_value = {};

		*_imvalue = ImPlot::MkGmtTime(_value.get());
	}
}