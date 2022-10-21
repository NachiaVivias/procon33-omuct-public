#pragma once
#include "../SivImGui/Widgets/Widget.hpp"

namespace SivImGui
{
	class SimpleSlider : public Widget
	{
		SIVIMGUI_BUILDER_HELPER(SimpleSlider);

		static SimpleSlider& New(Builder& ctx, const StringView text)
		{
			auto& w = ctx.next<SimpleSlider>();
			w.text = text;
			return w;
		}

		static const WidgetTypeInfo& TypeInfo()
		{
			const static WidgetTypeInfo info{
				.id = typeid(SimpleSlider).hash_code(),
				.name = U"SimpleButton",
			};
			return info;
		}

	public:

		SimpleSlider()
			: Widget(TypeInfo()) { }

	public:

		Property<String> text{ this, U"", PropertyFlag::Layout };

	public:

		bool clicked() const { return m_clicked; }
		double getSliderValue() const { return m_value; }

	protected:

		bool m_clicked = false;
		mutable double m_value = 0.0;

		virtual Size region() const override
		{
			// label = *text
			return SimpleGUI::SliderRegion({0, 0}).size.asPoint();
		}

		virtual void update(Rect rect) override
		{
			const Rect btnRect(rect.pos, rect.w, measuredSize().minSize.y);

			m_clicked = mouseOver() && btnRect.leftClicked();
		}

		virtual void draw(Rect rect) const override
		{
			double labelWidth = 80.0;
			[[maybe_unused]]
			bool _ = SimpleGUI::Slider(*text, m_value, (Vec2)rect.pos, labelWidth, (double)rect.w - labelWidth, isEnabled());
		}
	};
}
