#pragma once

#include<Siv3D.hpp>
#include "../SivImGui/Widgets/Widget.hpp"

namespace SivImGui {

	class ProgressBar :public Widget {

		SIVIMGUI_BUILDER_HELPER(ProgressBar);

		static ProgressBar& New(Builder& ctx)
		{
			auto& w = ctx.next<ProgressBar>();
			return w;
		}

		static const WidgetTypeInfo& TypeInfo()
		{
			const static WidgetTypeInfo info{
				.id = typeid(ProgressBar).hash_code(),
				.name = U"ProgressBar",
				.enableMouseOver = false
			};
			return info;
		}

	public:

		ProgressBar()
			: Widget(TypeInfo()) { }

	public:
		double progress = 0;
		Property<ColorF> backColor{ this, Palette::White, PropertyFlag::Layout };

		Property<ColorF> frameColor{ this, Palette::Black, PropertyFlag::Layout };

		Property<ColorF> progressColor{ this, Palette::Lime, PropertyFlag::Layout };

		Property<int32> frameThickness{ this, 1, PropertyFlag::Layout };

	protected:
		virtual Size region() const override
		{
			return SimpleGUI::SliderRegion({0,0}).size.asPoint();
		}

		virtual void draw(Rect rect) const override
		{
			rect.draw(backColor);

			Rect progressRect(rect.x,rect.y, (Rect::value_type)(rect.w * std::min(progress, 1.0)), rect.h);

			progressRect.draw(progressColor);

			rect.drawFrame(frameThickness, 0, frameColor);
		}
	};
}
