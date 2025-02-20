#include "mvDrag.h"
#include <utility>
#include "mvContext.h"
#include "dearpygui.h"
#include <string>
#include "mvItemRegistry.h"
#include "mvPythonExceptions.h"
#include "AppItems/fonts/mvFont.h"
#include "AppItems/themes/mvTheme.h"
#include "AppItems/containers/mvDragPayload.h"
#include "mvPyObject.h"

namespace Marvel {

    mvDragFloatMulti::mvDragFloatMulti(mvUUID uuid)
        : mvAppItem(uuid)
    {
    }

    void mvDragFloatMulti::applySpecificTemplate(mvAppItem* item)
    {
        auto titem = static_cast<mvDragFloatMulti*>(item);
        if(_source != 0) _value = titem->_value;
        _disabled_value[0] = titem->_disabled_value[0];
        _disabled_value[1] = titem->_disabled_value[1];
        _disabled_value[2] = titem->_disabled_value[2];
        _disabled_value[3] = titem->_disabled_value[3];
       _speed = titem->_speed;
       _min = titem->_min;
       _max = titem->_max;
       _format = titem->_format;
       _flags = titem->_flags;
       _stor_flags = titem->_stor_flags;
       _size = titem->_size;
    }

    PyObject* mvDragFloatMulti::getPyValue()
    {
        return ToPyFloatList(_value->data(), 4);
    }

    void mvDragFloatMulti::setPyValue(PyObject* value)
    {
        std::vector<float> temp = ToFloatVect(value);
        while (temp.size() < 4)
            temp.push_back(0.0f);
        std::array<float, 4> temp_array;
        for (size_t i = 0; i < temp_array.size(); i++)
            temp_array[i] = temp[i];
        if (_value)
            *_value = temp_array;
        else
            _value = std::make_shared<std::array<float, 4>>(temp_array);
    }

    void mvDragFloatMulti::setDataSource(mvUUID dataSource)
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
        _value = *static_cast<std::shared_ptr<std::array<float, 4>>*>(item->getValue());
    }

    void mvDragFloatMulti::draw(ImDrawList* drawlist, float x, float y)
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

            bool activated = false;

            if (!_enabled) std::copy(_value->data(), _value->data() + 2, _disabled_value);

            switch (_size)
            {
            case 2:
                activated = ImGui::DragFloat2(_internalLabel.c_str(), _enabled ? _value->data() : &_disabled_value[0], _speed, _min, _max, _format.c_str(), _flags);
                break;
            case 3:
                activated = ImGui::DragFloat3(_internalLabel.c_str(), _enabled ? _value->data() : &_disabled_value[0], _speed, _min, _max, _format.c_str(), _flags);
                break;
            case 4:
                activated = ImGui::DragFloat4(_internalLabel.c_str(), _enabled ? _value->data() : &_disabled_value[0], _speed, _min, _max, _format.c_str(), _flags);
                break;
            default:
                break;
            }

            if (activated)
            {
                auto value = *_value;
                if (_alias.empty())
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _uuid, ToPyFloatList(value.data(), (int)value.size()), _user_data);
                        });
                else
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _alias, ToPyFloatList(value.data(), (int)value.size()), _user_data);
                        });
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

    void mvDragFloatMulti::handleSpecificKeywordArgs(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        if (PyObject* item = PyDict_GetItemString(dict, "format")) _format = ToString(item);
        if (PyObject* item = PyDict_GetItemString(dict, "speed")) _speed = ToFloat(item);
        if (PyObject* item = PyDict_GetItemString(dict, "min_value")) _min = ToFloat(item);
        if (PyObject* item = PyDict_GetItemString(dict, "max_value")) _max = ToFloat(item);
        if (PyObject* item = PyDict_GetItemString(dict, "size")) _size = ToInt(item);

        // helper for bit flipping
        auto flagop = [dict](const char* keyword, int flag, int& flags)
        {
            if (PyObject* item = PyDict_GetItemString(dict, keyword)) ToBool(item) ? flags |= flag : flags &= ~flag;
        };

        // flags
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _stor_flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _stor_flags);

        if (_enabledLastFrame)
        {
            _enabledLastFrame = false;
            _flags = _stor_flags;
        }

        if (_disabledLastFrame)
        {
            _disabledLastFrame = false;
            _stor_flags = _flags;
            _flags |= ImGuiSliderFlags_NoInput;
        }

    }

    void mvDragFloatMulti::getSpecificConfiguration(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        mvPyObject py_format = ToPyString(_format);
        mvPyObject py_speed = ToPyFloat(_speed);
        mvPyObject py_min_value = ToPyFloat(_min);
        mvPyObject py_max_value = ToPyFloat(_max);
        mvPyObject py_size = ToPyInt(_size);

        PyDict_SetItemString(dict, "format", py_format);
        PyDict_SetItemString(dict, "speed", py_speed);
        PyDict_SetItemString(dict, "min_value", py_min_value);
        PyDict_SetItemString(dict, "max_value", py_max_value);
        PyDict_SetItemString(dict, "size", py_size);

        // helper to check and set bit
        auto checkbitset = [dict](const char* keyword, int flag, const int& flags)
        {
            mvPyObject py_result = ToPyBool(flags & flag);
            PyDict_SetItemString(dict, keyword, py_result);
        };

        // window flags
        checkbitset("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        checkbitset("no_input", ImGuiSliderFlags_NoInput, _flags);

    }

    mvDragIntMulti::mvDragIntMulti(mvUUID uuid)
        : mvAppItem(uuid)
    {
    }

    void mvDragIntMulti::applySpecificTemplate(mvAppItem* item)
    {
        auto titem = static_cast<mvDragIntMulti*>(item);
        if(_source != 0) _value = titem->_value;
        _disabled_value[0] = titem->_disabled_value[0];
        _disabled_value[1] = titem->_disabled_value[1];
        _disabled_value[2] = titem->_disabled_value[2];
        _disabled_value[3] = titem->_disabled_value[3];
        _speed = titem->_speed;
        _min = titem->_min;
        _max = titem->_max;
        _format = titem->_format;
        _flags = titem->_flags;
        _stor_flags = titem->_stor_flags;
        _size = titem->_size;
    }

    PyObject* mvDragIntMulti::getPyValue()
    {
        return ToPyIntList(_value->data(), 4);
    }

    void mvDragIntMulti::setPyValue(PyObject* value)
    {
        std::vector<int> temp = ToIntVect(value);
        while (temp.size() < 4)
            temp.push_back(0);
        std::array<int, 4> temp_array;
        for (size_t i = 0; i < temp_array.size(); i++)
            temp_array[i] = temp[i];
        if (_value)
            *_value = temp_array;
        else
            _value = std::make_shared<std::array<int, 4>>(temp_array);
    }

    void mvDragIntMulti::setDataSource(mvUUID dataSource)
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
        _value = *static_cast<std::shared_ptr<std::array<int, 4>>*>(item->getValue());
    }

    void mvDragIntMulti::draw(ImDrawList* drawlist, float x, float y)
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

            bool activated = false;

            if (!_enabled) std::copy(_value->data(), _value->data() + 2, _disabled_value);

            switch (_size)
            {
            case 2:
                activated = ImGui::DragInt2(_internalLabel.c_str(), _enabled ? _value->data() : &_disabled_value[0], _speed, _min, _max, _format.c_str(), _flags);
                break;
            case 3:
                activated = ImGui::DragInt3(_internalLabel.c_str(), _enabled ? _value->data() : &_disabled_value[0], _speed, _min, _max, _format.c_str(), _flags);
                break;
            case 4:
                activated = ImGui::DragInt4(_internalLabel.c_str(), _enabled ? _value->data() : &_disabled_value[0], _speed, _min, _max, _format.c_str(), _flags);
                break;
            default:
                break;
            }

            if (activated)
            {
                auto value = *_value;
                if (_alias.empty())
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _uuid, ToPyIntList(value.data(), (int)value.size()), _user_data);
                        });
                else
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _alias, ToPyIntList(value.data(), (int)value.size()), _user_data);
                        });
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
                    mvAddCallback(_dropCallback,_uuid, payloadActual->getDragData(), nullptr);
                }

                ImGui::EndDragDropTarget();
            }
        }
    }

    void mvDragIntMulti::handleSpecificKeywordArgs(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        if (PyObject* item = PyDict_GetItemString(dict, "format")) _format = ToString(item);
        if (PyObject* item = PyDict_GetItemString(dict, "speed")) _speed = ToFloat(item);
        if (PyObject* item = PyDict_GetItemString(dict, "min_value")) _min = ToInt(item);
        if (PyObject* item = PyDict_GetItemString(dict, "max_value")) _max = ToInt(item);
        if (PyObject* item = PyDict_GetItemString(dict, "size")) _size = ToInt(item);

        // helper for bit flipping
        auto flagop = [dict](const char* keyword, int flag, int& flags)
        {
            if (PyObject* item = PyDict_GetItemString(dict, keyword)) ToBool(item) ? flags |= flag : flags &= ~flag;
        };

        // flags
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _stor_flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _stor_flags);

        if (_enabledLastFrame)
        {
            _enabledLastFrame = false;
            _flags = _stor_flags;
        }

        if (_disabledLastFrame)
        {
            _disabledLastFrame = false;
            _stor_flags = _flags;
            _flags |= ImGuiSliderFlags_NoInput;
        }

    }

    void mvDragIntMulti::getSpecificConfiguration(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        mvPyObject py_format = ToPyString(_format);
        mvPyObject py_speed = ToPyFloat(_speed);
        mvPyObject py_min_value = ToPyInt(_min);
        mvPyObject py_max_value = ToPyInt(_max);
        mvPyObject py_size = ToPyInt(_size);

        PyDict_SetItemString(dict, "format", py_format);
        PyDict_SetItemString(dict, "speed", py_speed);
        PyDict_SetItemString(dict, "min_value", py_min_value);
        PyDict_SetItemString(dict, "max_value", py_max_value);
        PyDict_SetItemString(dict, "size", py_size);

        // helper to check and set bit
        auto checkbitset = [dict](const char* keyword, int flag, const int& flags)
        {
            mvPyObject py_result = ToPyBool(flags & flag);
            PyDict_SetItemString(dict, keyword, py_result);
        };

        // window flags
        checkbitset("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        checkbitset("no_input", ImGuiSliderFlags_NoInput, _flags);

    }

    mvDragFloat::mvDragFloat(mvUUID uuid)
        : mvAppItem(uuid)
    {
    }

    void mvDragFloat::applySpecificTemplate(mvAppItem* item)
    {
        auto titem = static_cast<mvDragFloat*>(item);
        if (_source != 0) _value = titem->_value;
        _disabled_value = titem->_disabled_value;
        _speed = titem->_speed;
        _min = titem->_min;
        _max = titem->_max;
        _format = titem->_format;
        _flags = titem->_flags;
        _stor_flags = titem->_stor_flags;
    }

    PyObject* mvDragFloat::getPyValue()
    {
        return ToPyFloat(*_value);
    }

    void mvDragFloat::setPyValue(PyObject* value)
    {
        *_value = ToFloat(value);
    }

    void mvDragFloat::setDataSource(mvUUID dataSource)
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
        _value = *static_cast<std::shared_ptr<float>*>(item->getValue());
    }

    void mvDragFloat::draw(ImDrawList* drawlist, float x, float y)
    {
        ScopedID id(_uuid);


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

            if (!_enabled) _disabled_value = *_value;

            if (ImGui::DragFloat(_internalLabel.c_str(), _enabled ? _value.get() : &_disabled_value, _speed, _min, _max, _format.c_str(), _flags))
            {
                auto value = *_value;

                if (_alias.empty())
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _uuid, ToPyFloat(value), _user_data);
                        });
                else
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _alias, ToPyFloat(value), _user_data);
                        });
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
                        mvAddCallback(_dropCallback, _uuid, payloadActual->getDragData(), nullptr);
                    else
                        mvAddCallback(_dropCallback, _alias, payloadActual->getDragData(), nullptr);
                }

                ImGui::EndDragDropTarget();
            }
        }
    }

    mvDragInt::mvDragInt(mvUUID uuid)
        : mvAppItem(uuid)
    {
    }

    void mvDragInt::applySpecificTemplate(mvAppItem* item)
    {
        auto titem = static_cast<mvDragInt*>(item);
        if (_source != 0) _value = titem->_value;
        _disabled_value = titem->_disabled_value;
        _speed = titem->_speed;
        _min = titem->_min;
        _max = titem->_max;
        _format = titem->_format;
        _flags = titem->_flags;
        _stor_flags = titem->_stor_flags;
    }

    void mvDragInt::setDataSource(mvUUID dataSource)
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
        _value = *static_cast<std::shared_ptr<int>*>(item->getValue());
    }

    PyObject* mvDragInt::getPyValue()
    {
        return ToPyInt(*_value);
    }

    void mvDragInt::setPyValue(PyObject* value)
    {
        *_value = ToInt(value);
    }

    void mvDragInt::draw(ImDrawList* drawlist, float x, float y)
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

            if (!_enabled) _disabled_value = *_value;

            if (ImGui::DragInt(_internalLabel.c_str(), _enabled ? _value.get() : &_disabled_value, _speed, _min, _max, _format.c_str(), _flags))
            {
                auto value = *_value;
                if (_alias.empty())
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _uuid, ToPyInt(value), _user_data);
                        });
                else
                    mvSubmitCallback([=]() {
                    mvAddCallback(getCallback(false), _alias, ToPyInt(value), _user_data);
                        });
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
                        mvAddCallback(_dropCallback, _uuid, payloadActual->getDragData(), nullptr);
                    else
                        mvAddCallback(_dropCallback, _alias, payloadActual->getDragData(), nullptr);
                }

                ImGui::EndDragDropTarget();
            }
        }
    }

    void mvDragFloat::handleSpecificKeywordArgs(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        if (PyObject* item = PyDict_GetItemString(dict, "format")) _format = ToString(item);
        if (PyObject* item = PyDict_GetItemString(dict, "speed")) _speed = ToFloat(item);
        if (PyObject* item = PyDict_GetItemString(dict, "min_value")) _min = ToFloat(item);
        if (PyObject* item = PyDict_GetItemString(dict, "max_value")) _max = ToFloat(item);

        // helper for bit flipping
        auto flagop = [dict](const char* keyword, int flag, int& flags)
        {
            if (PyObject* item = PyDict_GetItemString(dict, keyword)) ToBool(item) ? flags |= flag : flags &= ~flag;
        };

        // flags
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _stor_flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _stor_flags);

        if (_enabledLastFrame)
        {
            _enabledLastFrame = false;
            _flags = _stor_flags;
        }

        if (_disabledLastFrame)
        {
            _disabledLastFrame = false;
            _stor_flags = _flags;
            _flags |= ImGuiSliderFlags_NoInput;
        }

    }

    void mvDragFloat::getSpecificConfiguration(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        mvPyObject py_format = ToPyString(_format);
        mvPyObject py_speed = ToPyFloat(_speed);
        mvPyObject py_min_value = ToPyFloat(_min);
        mvPyObject py_max_value = ToPyFloat(_max);

        PyDict_SetItemString(dict, "format", py_format);
        PyDict_SetItemString(dict, "speed", py_speed);
        PyDict_SetItemString(dict, "min_value", py_min_value);
        PyDict_SetItemString(dict, "max_value", py_max_value);

        // helper to check and set bit
        auto checkbitset = [dict](const char* keyword, int flag, const int& flags)
        {
            mvPyObject py_result = ToPyBool(flags & flag);
            PyDict_SetItemString(dict, keyword, py_result);
        };

        // window flags
        checkbitset("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        checkbitset("no_input", ImGuiSliderFlags_NoInput, _flags);

    }

    void mvDragInt::handleSpecificKeywordArgs(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        if (PyObject* item = PyDict_GetItemString(dict, "format")) _format = ToString(item);
        if (PyObject* item = PyDict_GetItemString(dict, "speed")) _speed = ToFloat(item);
        if (PyObject* item = PyDict_GetItemString(dict, "min_value")) _min = ToInt(item);
        if (PyObject* item = PyDict_GetItemString(dict, "max_value")) _max = ToInt(item);

        // helper for bit flipping
        auto flagop = [dict](const char* keyword, int flag, int& flags)
        {
            if (PyObject* item = PyDict_GetItemString(dict, keyword)) ToBool(item) ? flags |= flag : flags &= ~flag;
        };

        // flags
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        flagop("clamped", ImGuiSliderFlags_ClampOnInput, _stor_flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _flags);
        flagop("no_input", ImGuiSliderFlags_NoInput, _stor_flags);

        if (_enabledLastFrame)
        {
            _enabledLastFrame = false;
            _flags = _stor_flags;
        }

        if (_disabledLastFrame)
        {
            _disabledLastFrame = false;
            _stor_flags = _flags;
            _flags |= ImGuiSliderFlags_NoInput;
        }
    }

    void mvDragInt::getSpecificConfiguration(PyObject* dict)
    {
        if (dict == nullptr)
            return;

        mvPyObject py_format = ToPyString(_format);
        mvPyObject py_speed = ToPyFloat(_speed);
        mvPyObject py_min_value = ToPyInt(_min);
        mvPyObject py_max_value = ToPyInt(_max);

        PyDict_SetItemString(dict, "format", py_format);
        PyDict_SetItemString(dict, "speed", py_speed);
        PyDict_SetItemString(dict, "min_value", py_min_value);
        PyDict_SetItemString(dict, "max_value", py_max_value);

        // helper to check and set bit
        auto checkbitset = [dict](const char* keyword, int flag, const int& flags)
        {
            mvPyObject py_result = ToPyBool(flags & flag);
            PyDict_SetItemString(dict, keyword, py_result);
        };

        // window flags
        checkbitset("clamped", ImGuiSliderFlags_ClampOnInput, _flags);
        checkbitset("no_input", ImGuiSliderFlags_NoInput, _flags);

    }

}