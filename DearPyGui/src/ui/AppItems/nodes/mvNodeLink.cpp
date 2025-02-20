#include "mvNodeLink.h"
#include <imnodes.h>
#include "mvContext.h"
#include "mvLog.h"
#include "mvNodeAttribute.h"
#include "mvItemRegistry.h"
#include "mvPythonExceptions.h"
#include "AppItems/fonts/mvFont.h"
#include "AppItems/themes/mvTheme.h"
#include "AppItems/containers/mvDragPayload.h"
#include "mvPyObject.h"

namespace Marvel {

	mvNodeLink::mvNodeLink(mvUUID uuid)
		: mvAppItem(uuid)
	{
		int64_t address = (int64_t)this;
		int64_t reduced_address = address % 2147483648;
		_id0 = (int)reduced_address;
	}

	void mvNodeLink::handleSpecificRequiredArgs(PyObject* dict)
	{
		if (!VerifyRequiredArguments(GetParsers()[GetEntityCommand(_type)], dict))
			return;

		for (int i = 0; i < PyTuple_Size(dict); i++)
		{
			PyObject* item = PyTuple_GetItem(dict, i);
			switch (i)
			{
			case 0:
			{
				mvUUID node1 = GetIDFromPyObject(item);
				mvAppItem* node = GetItem(*GContext->itemRegistry, node1);
				if (node->_type == mvAppItemType::mvNodeAttribute)
				{
					_id1uuid = node1;
					_id1 = static_cast<const mvNodeAttribute*>(node)->getId();
				}
				else
				{
					mvThrowPythonError(mvErrorCode::mvIncompatibleType, GetEntityCommand(_type),
						"Incompatible type. Expected types include: mvNode", node);
					MV_ITEM_REGISTRY_ERROR("Nodes must be nodes. duh");
					assert(false);
					return;
				}
				break;
			}

			case 1:
			{
				mvUUID node2 = GetIDFromPyObject(item);
				mvAppItem* node = GetItem(*GContext->itemRegistry, node2);
				if (node->_type == mvAppItemType::mvNodeAttribute)
				{
					_id2uuid = node2;
					_id2 = static_cast<const mvNodeAttribute*>(node)->getId();
				}
				else
				{
					mvThrowPythonError(mvErrorCode::mvIncompatibleType, GetEntityCommand(_type),
						"Incompatible type. Expected types include: mvNode", node);
					MV_ITEM_REGISTRY_ERROR("Nodes must be nodes. duh");
					assert(false);
					return;
				}
				break;
			}

			default:
				break;
			}
		}
	}

	void mvNodeLink::customAction(void* data)
	{
		if (_handlerRegistry)
			_handlerRegistry->customAction(data);
	}

	void mvNodeLink::draw(ImDrawList* drawlist, float x, float y)
	{

		//-----------------------------------------------------------------------------
		// pre draw
		//-----------------------------------------------------------------------------

		// show/hide
		if (!_show)
			return;

		// set item width
		if (_width != 0)
			ImGui::SetNextItemWidth((float)_width);

		// themes
		apply_local_theming(this);

		//-----------------------------------------------------------------------------
		// draw
		//-----------------------------------------------------------------------------
		ScopedID id(_uuid);

		imnodes::Link(_id0, _id1, _id2);

		//-----------------------------------------------------------------------------
		// post draw
		//-----------------------------------------------------------------------------
		_state.lastFrameUpdate = GContext->frame;

		//-----------------------------------------------------------------------------
		// update state
		//   * only update if applicable
		//-----------------------------------------------------------------------------
		//_state.hovered = imnodes::IsLinkHovered(&_id);
		_state.visible = ImGui::IsItemVisible();
		//_state.active = imnodes::IsLinkStarted(&_id);
		//_state.deactivated = imnodes::IsLinkDropped(&_id);

		// handle popping themes
		if (_enabled)
		{
			if (auto classTheme = GetClassThemeComponent(_type))
				static_cast<mvThemeComponent*>(classTheme.get())->customAction();
		}
		else
		{
			if (auto classTheme = GetDisabledClassThemeComponent(_type))
				static_cast<mvThemeComponent*>(classTheme.get())->customAction();
		}

		if (_theme)
		{
			static_cast<mvTheme*>(_theme.get())->setSpecificEnabled(_enabled);
			static_cast<mvTheme*>(_theme.get())->setSpecificType((int)_type);
			static_cast<mvTheme*>(_theme.get())->customAction();
		}
	}

	void mvNodeLink::getSpecificConfiguration(PyObject* dict)
	{
		if (dict == nullptr)
			return;

		PyDict_SetItemString(dict, "attr_1", mvPyObject(ToPyUUID(_id1uuid)));
		PyDict_SetItemString(dict, "attr_2", mvPyObject(ToPyUUID(_id2uuid)));
	}

}