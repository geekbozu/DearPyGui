#include "mvFontRegistry.h"
#include "mvFont.h"
#include "mvPythonExceptions.h"
#include "mvLog.h"
#include "mvToolManager.h"
#include "mvFontManager.h"

namespace Marvel {

	mvFontRegistry::mvFontRegistry(mvUUID uuid)
		:
		mvAppItem(uuid)
	{
		_show = true;
	}

	void mvFontRegistry::resetFont()
	{
		for (auto& item : _children[1])
		{
			static_cast<mvFont*>(item.get())->_default = false;
		}

		mvToolManager::GetFontManager()._resetDefault = true;
	}

	void mvFontRegistry::draw(ImDrawList* drawlist, float x, float y)
	{
		//ImGuiIO& io = ImGui::GetIO();
		//io.Fonts->Clear();
		//io.FontDefault = io.Fonts->AddFontDefault();

		for (auto& item : _children[1])
		{
			item->draw(drawlist, ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
		}
		_show = false;
	}

	void mvFontRegistry::customAction(void* data)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		io.FontDefault = io.Fonts->AddFontDefault();

		for (auto& item : _children[1])
		{
			item->customAction(data);
		}
	}

}