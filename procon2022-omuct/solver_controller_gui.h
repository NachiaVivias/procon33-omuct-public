#pragma once
#include "global.h"
#include "solver_controller.h"

namespace Procon33 {


	class SolverControllerGui :public SivImGui::Widget {
	public:

		SIVIMGUI_BUILDER_HELPER(SolverControllerGui);

		static SolverControllerGui& New(SivImGui::Builder& ctx)
		{
			auto& w = ctx.next<SolverControllerGui>();
			return w;
		}

		static const SivImGui::WidgetTypeInfo& TypeInfo()
		{
			const static SivImGui::WidgetTypeInfo info{
				.id = typeid(SolverControllerGui).hash_code(),
				.name = U"SolverControllerGui",
				.enableMouseOver = false
			};
			return info;
		}

		SolverControllerGui() : m_controller(Global::GetInstance()->solverControl)
		{}

		static void MainFunctionUnitTest();

	private:

		struct HighlightState {
			double duration;
			double decrease;
			Color color;
		};

		int32 HIGHLINGHT_DURATION = 100;
		std::shared_ptr<SolverController> m_controller;
		std::map<String, HighlightState> m_highlightTime;

		String m_precalcId = U"";

	protected:


		virtual Size region() const override
		{
			// label = *text
			return Size(600, 400);
		}

		virtual void update(Rect rect) override;

		virtual void draw(Rect rect) const override;

	};

}
