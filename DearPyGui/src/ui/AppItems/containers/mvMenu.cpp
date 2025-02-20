#include "mvMenu.h"
#include "mvContext.h"
#include "mvItemRegistry.h"
#include "mvPythonExceptions.h"
#include "AppItems/fonts/mvFont.h"
#include "AppItems/themes/mvTheme.h"
#include "AppItems/containers/mvDragPayload.h"

namespace Marvel {

	mvMenu::mvMenu(mvUUID uuid)
		: mvAppItem(uuid)
	{
	}

	void mvMenu::draw(ImDrawList* drawlist, float x, float y)
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

			// create menu and see if its selected
			if (ImGui::BeginMenu(_internalLabel.c_str(), _enabled))
			{
				_state.lastFrameUpdate = GContext->frame;
				_state.active = ImGui::IsItemActive();
				_state.activated = ImGui::IsItemActivated();
				_state.deactivated = ImGui::IsItemDeactivated();
				_state.focused = ImGui::IsWindowFocused();
				_state.hovered = ImGui::IsWindowHovered();
				_state.rectSize = { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() };
				_state.contextRegionAvail = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };

				// set other menus's value false on same level
				for (auto& sibling : _parentPtr->_children[1])
				{
					// ensure sibling
					if (sibling->_type == mvAppItemType::mvMenu)
						*((mvMenu*)sibling.get())->_value = false;
				}

				// set current menu value true
				*_value = true;

				for (auto& item : _children[1])
					item->draw(drawlist, ImGui::GetCursorPosX(), ImGui::GetCursorPosY());

				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
				{

					// update mouse
					ImVec2 mousePos = ImGui::GetMousePos();
					float x = mousePos.x - ImGui::GetWindowPos().x;
					float y = mousePos.y - ImGui::GetWindowPos().y;
					GContext->input.mousePos.x = (int)x;
					GContext->input.mousePos.y = (int)y;


					if (GContext->itemRegistry->activeWindow != _uuid)
						GContext->itemRegistry->activeWindow = _uuid;


				}

				ImGui::EndMenu();
			}
		}

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

	void mvMenu::handleSpecificKeywordArgs(PyObject* dict)
	{
		if (dict == nullptr)
			return;
		 
		if (PyObject* item = PyDict_GetItemString(dict, "enabled")) _enabled = ToBool(item);

	}

	void mvMenu::getSpecificConfiguration(PyObject* dict)
	{
		if (dict == nullptr)
			return;
		 
		PyDict_SetItemString(dict, "enabled", mvPyObject(ToPyBool(_enabled)));
	}

	PyObject* mvMenu::getPyValue()
	{
		return ToPyBool(*_value);
	}

	void mvMenu::setPyValue(PyObject* value)
	{
		*_value = ToBool(value);
	}

	void mvMenu::setDataSource(mvUUID dataSource)
	{
		if (dataSource == _source) return;
		_source = dataSource;

		mvAppItem* item = GetItem((*GContext->itemRegistry), dataSource);
		if (!item)
		{
			mvThrowPythonError(mvErrorCode::mvSourceNotFound, "set_value",
				"Source item not found: " + std::to_string(dataSource), this);
			return;
		}
		if (GetEntityValueType(item->_type) != GetEntityValueType(_type))
		{
			mvThrowPythonError(mvErrorCode::mvSourceNotCompatible, "set_value",
				"Values types do not match: " + std::to_string(dataSource), this);
			return;
		}
		_value = *static_cast<std::shared_ptr<bool>*>(item->getValue());
	}

}