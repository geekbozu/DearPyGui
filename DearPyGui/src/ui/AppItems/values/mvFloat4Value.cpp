#include "mvFloat4Value.h"
#include <utility>
#include "mvContext.h"
#include "dearpygui.h"
#include <string>
#include "mvPythonExceptions.h"

namespace Marvel {

    mvFloat4Value::mvFloat4Value(mvUUID uuid)
        : mvAppItem(uuid)
    {
    }

	PyObject* mvFloat4Value::getPyValue()
	{
		return ToPyFloatList(_value->data(), 4);
	}

	void mvFloat4Value::setPyValue(PyObject* value)
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

	void mvFloat4Value::setDataSource(mvUUID dataSource)
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

}
