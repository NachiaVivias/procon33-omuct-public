#pragma once

namespace SivImGui
{
	class WidgetBase;
}

#include "WidgetBase.hpp"

namespace SivImGui
{
	enum class PropertyFlag
	{
		None = 0,
		Renderer = 0b01,
		Layout = 0b10,
	};
	DEFINE_BITMASK_OPERATORS(PropertyFlag);

	template<class T>
	class Property
	{
	public:

		template<class WidgetT>
		Property(
			WidgetT* _this,
			const T defaultValue,
			PropertyFlag flag = PropertyFlag::None,
			[[maybe_unused]] const StringView name = U""
		)
			: m_value(defaultValue)
			, m_widget(*_this)
			, m_flag(flag)
		{ }

	public:

		const T& value() const { return m_value; }

		template<class U = T> requires std::equality_comparable_with<T, U>
		const T& operator =(U value)
		{
			if (m_value != value)
			{
				if (m_flag & PropertyFlag::Layout)
				{
					m_widget.requestLayout();
				}
				return m_value = value;
			}
			return m_value;
		}

		const T& operator =(T value)
		{
			if (m_value != value)
			{
				if (m_flag & PropertyFlag::Layout)
				{
					m_widget.requestLayout();
				}
				return m_value = value;
			}
			return m_value;
		}

		const T& operator*() const { return m_value; }

		const T* operator->() const { return &m_value; }

		operator T() const { return m_value; }

	private:

		const PropertyFlag m_flag;

		WidgetBase& m_widget;

		T m_value;
	};
}

#define SIVIMGUI_PROPERTY(TYPE, NAME, VALUE)\
Property<TYPE> NAME{ this, VALUE, PropertyFlag::None, U"" #NAME}

#define SIVIMGUI_LAYOUT_PROPERTY(TYPE, NAME, VALUE)\
Property<TYPE> NAME{ this, VALUE, PropertyFlag::Layout, U"" #NAME}
