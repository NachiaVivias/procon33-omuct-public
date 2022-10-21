#pragma once
#include "../SivImGui/Widgets/Widget.hpp"

namespace SivImGui
{
	class SimpleTextBox : public Widget
	{
		SIVIMGUI_BUILDER_HELPER(SimpleTextBox);

		static SimpleTextBox& New(Builder& ctx, const StringView text)
		{
			auto& w = ctx.next<SimpleTextBox>();
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

		SimpleTextBox()
			: Widget(TypeInfo()) { }

	public:

		Property<String> text{ this, U"", PropertyFlag::Layout };

	public:

		bool clicked() const { return m_clicked; }
		const String& getContent() const { return m_content.text; }

	protected:

		bool m_clicked = false;
		mutable TextEditState m_content = TextEditState(U"");

		virtual Size region() const override
		{
			// label = *text
			return SimpleGUI::TextBoxRegion({ 0, 0 }).size.asPoint();
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
			bool _ = SimpleGUI::TextBox(m_content, (Vec2)rect.pos, (double)rect.w - labelWidth, unspecified, isEnabled());
		}
	};
}
