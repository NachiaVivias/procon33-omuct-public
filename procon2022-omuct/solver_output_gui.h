#pragma once

#include "solver/solver_interface.h"

namespace Procon33 {

	class SolverOutputGui :public SivImGui::Widget {
	public:

		SIVIMGUI_BUILDER_HELPER(SolverOutputGui);

		static SolverOutputGui& New(SivImGui::Builder& ctx)
		{
			auto& w = ctx.next<SolverOutputGui>();
			return w;
		}

		static const SivImGui::WidgetTypeInfo& TypeInfo()
		{
			const static SivImGui::WidgetTypeInfo info{
				.id = typeid(SolverControllerGui).hash_code(),
				.name = U"SolverOutputGui",
				.enableMouseOver = false
			};
			return info;
		}

		SolverOutputGui() : m_solver(Global::GetInstance()->solver)
		{
			xExpand = true;
			yExpand = true;
		}

		static void MainFunctionUnitTest();

	private:

		std::shared_ptr<SolverInterface> m_solver;

	protected:


		virtual Size region() const override
		{
			// label = *text
			return Size(600, 400);
		}

		virtual void update(Rect) override {}

		virtual void draw(Rect rect) const {
			m_solver->draw(rect.stretched(-15));
		}

	};

} // namespace Procon33
