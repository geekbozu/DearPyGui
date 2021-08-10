#include "mvMouseDragHandler.h"
#include "mvLog.h"
#include "mvItemRegistry.h"
#include "mvPythonExceptions.h"
#include "mvUtilities.h"

namespace Marvel {

	void mvMouseDragHandler::InsertParser(std::map<std::string, mvPythonParser>* parsers)
	{

		mvPythonParser parser(mvPyDataType::UUID, "Adds a handler which runs a given callback when the specified mouse button is clicked and dragged a set threshold. Parent must be a handler registry.", { "Events", "Widgets" });
		mvAppItem::AddCommonArgs(parser, (CommonParserArgs)(
			MV_PARSER_ARG_ID |
			MV_PARSER_ARG_CALLBACK |
			MV_PARSER_ARG_SHOW)
		);

		parser.addArg<mvPyDataType::Integer>("button", mvArgType::POSITIONAL_ARG, "-1", "Submits callback for all mouse buttons");
		parser.addArg<mvPyDataType::Float>("threshold", mvArgType::POSITIONAL_ARG, "10.0", "The threshold the mouse must be dragged before the callback is ran");
		parser.addArg<mvPyDataType::UUID>("parent", mvArgType::KEYWORD_ARG, "internal_dpg.mvReservedUUID_1", "Parent to add this item to. (runtime adding)");
		parser.finalize();

		parsers->insert({ s_command, parser });
	}

	mvMouseDragHandler::mvMouseDragHandler(mvUUID uuid)
		:
		mvAppItem(uuid)
	{

	}

	void mvMouseDragHandler::draw(ImDrawList* drawlist, float x, float y)
	{
		if (_button == -1)
		{
			for (int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().MouseDown); i++)
			{
				if (ImGui::IsMouseReleased(i))
					ImGui::ResetMouseDragDelta(i);

				if (ImGui::IsMouseDragging(i, _threshold))
				{
					mvApp::GetApp()->getCallbackRegistry().submitCallback([=]()
						{
							mvApp::GetApp()->getCallbackRegistry().runCallback(getCallback(false), _uuid,
								ToPyMTrip(i, ImGui::GetMouseDragDelta(i).x, ImGui::GetMouseDragDelta(i).y), _user_data);
						});
				}
			}
		}

		else if (ImGui::IsMouseDragging(_button, _threshold))
		{
			if (ImGui::IsMouseReleased(_button))
				ImGui::ResetMouseDragDelta(_button);
			mvApp::GetApp()->getCallbackRegistry().submitCallback([=]()
				{
					mvApp::GetApp()->getCallbackRegistry().runCallback(getCallback(false), _uuid,
						ToPyMTrip(_button, ImGui::GetMouseDragDelta(_button).x, ImGui::GetMouseDragDelta(_button).y), _user_data);
				});
		}
	}

	void mvMouseDragHandler::handleSpecificPositionalArgs(PyObject* dict)
	{
		if (!mvApp::GetApp()->getParsers()[s_command].verifyRequiredArguments(dict))
			return;

		for (int i = 0; i < PyTuple_Size(dict); i++)
		{
			PyObject* item = PyTuple_GetItem(dict, i);
			switch (i)
			{
			case 0:
				_button = ToInt(item);
				break;
			case 1:
				_threshold = ToFloat(item);
				break;

			default:
				break;
			}
		}
	}

	void mvMouseDragHandler::handleSpecificKeywordArgs(PyObject* dict)
	{
		if (dict == nullptr)
			return;

		if (PyObject* item = PyDict_GetItemString(dict, "button")) _button = ToInt(item);
		if (PyObject* item = PyDict_GetItemString(dict, "threshold")) _threshold = ToFloat(item);
	}

	void mvMouseDragHandler::getSpecificConfiguration(PyObject* dict)
	{
		if (dict == nullptr)
			return;

		PyDict_SetItemString(dict, "button", ToPyInt(_button));
		PyDict_SetItemString(dict, "threshold", ToPyFloat(_threshold));
	}
}