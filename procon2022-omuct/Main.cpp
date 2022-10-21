

#include <Siv3D.hpp> // OpenSiv3D v0.6.5

#include "global.h"
#include "solver_controller_gui.h"
#include "solver_output_gui.h"
#include "match_manager_gui.h"
#include "resource_check.h"

namespace Procon33 {


	namespace {

		constexpr int32 FIXED_SCENE_WIDTH = 1280;
		constexpr int32 FIXED_SCENE_HEIGHT = 720;

		constexpr int32 GUI_HEADER_HEIGHT = 120;
		constexpr int32 GUI_QUESTION_SELECTOR_WIDTH = 300;

		constexpr Point GUI_MAIN_REGION_LEFTTOP = Point(GUI_QUESTION_SELECTOR_WIDTH, GUI_HEADER_HEIGHT);

		String GUI_ID_SOLVER_CONTROLLER = U"ソルバ操作";
		String GUI_ID_SOLVER_OUTPUT = U"ソルバ出力";
		String GUI_ID_SERVER_SETTING = U"サーバー設定";

		const Array<String> GUI_ID_LIST = {
			GUI_ID_SOLVER_CONTROLLER,
			GUI_ID_SOLVER_OUTPUT,
			GUI_ID_SERVER_SETTING,
			U"D"
		};

		Array<String> g_problemIds;

	}


	namespace {

		class HeaderGui {
		public:

			HeaderGui()
				: m_font(40)
			{
				if(GUI_ID_LIST.empty()) throw Error(U"グローバル変数 GUI_ID_LIST が空ですが、ダメです。");

				// GUI 切り替え欄と、問題セレクタを縦に並べる
				auto rootV = std::make_unique<SivImGui::Container>();
				rootV->layout = SivImGui::VerticalLayout{
					.padding = 0,
					.horizontalAlignment = SivImGui::Alignment::Start,
					.verticalAlignment = SivImGui::Alignment::Start
				};
				rootV->xExpand = true;
				rootV->yExpand = true;

				// ヘッダーの作成
				{
					auto headerColor = std::make_unique<SivImGui::Box>();
					headerColor->xExpand = true;
					headerColor->backColor = Palette::Purple;
					headerColor->layout = SivImGui::HorizontalLayout{
						.padding = 0,
						.horizontalAlignment = SivImGui::Alignment::Start,
						.verticalAlignment = SivImGui::Alignment::Start
					};
					headerColor->minSize = Size(0, GUI_HEADER_HEIGHT);

					auto header = std::make_unique<SivImGui::ScrollView>();

					header->xExpand = true;
					header->layout = SivImGui::HorizontalLayout{
						.horizontalAlignment = SivImGui::Alignment::Start,
						.verticalAlignment = SivImGui::Alignment::Start
					};
					header->minSize = Size(0, GUI_HEADER_HEIGHT);
					header->mode = SivImGui::ScrollView::Mode::Horizontal;

					auto AddGuiSelectButton = [&](String btnId, String btnText) -> SivImGui::Button* {
						//GUI切り替えボタンの作成
						auto changeGuiButton = std::make_unique<SivImGui::Button>();
						auto changeGuiLabel = std::make_unique<SivImGui::Label>();
						auto res = changeGuiButton.get();
						changeGuiLabel->text = btnText;
						changeGuiLabel->textColor = s3d::Palette::Black;
						changeGuiLabel->font = m_font;
						changeGuiButton->addChild(std::move(changeGuiLabel));
						header->addChild(std::move(changeGuiButton));
						return res;
					};

					for (auto& guiId : GUI_ID_LIST) {
						m_buttonReference.push_back(AddGuiSelectButton(guiId, guiId));
						m_mainGuis.emplace_back();
					}

					headerColor->addChild(std::move(header));
					rootV->addChild(std::move(headerColor));
				}


				//問題セレクタの作成
				{
					auto questionSelector = std::make_unique<SivImGui::Container>();
					questionSelector->layout = SivImGui::HorizontalLayout{
						.space = 20,
						.horizontalAlignment = SivImGui::Alignment::Start,
						.verticalAlignment = SivImGui::Alignment::Start
					};
					questionSelector->minSize = Size(GUI_QUESTION_SELECTOR_WIDTH, 0);
					questionSelector->yExpand = true;

					auto scrollView = std::make_unique<SivImGui::ScrollView>();
					scrollView->mode = SivImGui::ScrollView::Mode::Both;
					scrollView->minSize = Size(GUI_QUESTION_SELECTOR_WIDTH, 0);
					scrollView->yExpand = true;
					scrollView->layout = SivImGui::VerticalLayout{
						.horizontalAlignment = SivImGui::Alignment::Start,
						.verticalAlignment = SivImGui::Alignment::Start
					};
					scrollView->setDarkMode(true);
					m_scrollViewReference = scrollView.get();

					questionSelector->addChild(std::move(scrollView));

					rootV->addChild(std::move(questionSelector));
				}

				//ヘッダ、問題セレクタそれぞれでGUIを作成
				m_gui = std::make_unique<SivImGui::GUI>(std::move(rootV));
			}

			void layout(Size availableSize) {
				m_gui->layout(availableSize);

				if ((bool)m_mainGuis[m_currentGuiIndex]) {
					m_mainGuis[m_currentGuiIndex]->layout(availableSize - GUI_MAIN_REGION_LEFTTOP);
				}
			}

			void update(Point pos = { 0,0 }) {
				m_gui->update(pos);
				m_guiChanged = false;

				for (size_t i = 0; i < m_buttonReference.size(); i++) {
					if (m_buttonReference[i]->clicked() && m_currentGuiIndex != i) {
						m_guiChanged = true;
						m_currentGuiIndex = (int32)i;
					}
				}

				for (size_t i = 0; i < m_problemLabelReference.size(); i++) {
					if (m_problemLabelReference[i]->mouseOver() && MouseL.down()) {
						if (i < m_problemIds.size()) {
							m_currentProblemIndex = (int32)i;
							Global::GetInstance()->solverControl->setCurrentProblem(getCurrentProblemId());
						}
					}
				}

				if ((bool)m_mainGuis[m_currentGuiIndex]) {
					m_mainGuis[m_currentGuiIndex]->update(GUI_MAIN_REGION_LEFTTOP);
				}
			}

			void draw(Point pos = { 0,0 }) {
				m_gui->draw(pos);

				if ((bool)m_mainGuis[m_currentGuiIndex]) {
					m_mainGuis[m_currentGuiIndex]->draw(GUI_MAIN_REGION_LEFTTOP);
				}

				Line(Vec2(GUI_QUESTION_SELECTOR_WIDTH + 5, GUI_HEADER_HEIGHT + 15), Vec2(GUI_QUESTION_SELECTOR_WIDTH + 5, FIXED_SCENE_HEIGHT - 15)).draw(3.0, Palette::White.withAlpha(100));
			}

			const std::unique_ptr<SivImGui::GUI>& gui() const { return m_gui; }

			String getCurrentGuiId() const { return GUI_ID_LIST[m_currentGuiIndex]; }
			bool guiChanged() const { return m_guiChanged; }

			String getCurrentProblemId() const {
				if (m_currentProblemIndex < 0) return U"";
				if (m_currentProblemIndex >= (int32)m_problemIds.size()) return U"";
				return m_problemIds[m_currentProblemIndex];
			}
			bool problemChanged() const { return m_problemChanged; }
			void overwriteCurrentProblemId(String problemId) {
				m_currentProblemIndex = -1;
				for (int32 i = 0; i < (int32)m_problemIds.size(); i++) if (m_problemIds[i] == problemId) m_currentProblemIndex = i;
			}

			void setMainGui(String guiId, std::shared_ptr<SivImGui::GUI> gui) {
				size_t index = GuiIdToIndex(guiId);
				m_mainGuis[index] = gui;
			}
			void setProblemIds(Array<String> problemIds) {
				if (m_problemIds != problemIds) {
					std::swap(m_problemIds, problemIds);
					refreshProblemLabels();
				}
			}

		private:

			static size_t GuiIdToIndex(String id) {
				for (size_t i = 0; i < GUI_ID_LIST.size(); i++) if (GUI_ID_LIST[i] == id) return i;
				throw Error(U"HeaderGui::GuiIdToIndex で未登録の GUI ID を検出しました。 ： \"" + id + U"\"");
			}

			Array<SivImGui::Button*> m_buttonReference;
			Array<std::shared_ptr<SivImGui::GUI>> m_mainGuis;
			int32 m_currentGuiIndex = 0;
			bool m_guiChanged = true;

			SivImGui::ScrollView* m_scrollViewReference;
			Array<SivImGui::Label*> m_problemLabelReference;
			int32 m_currentProblemIndex = -1;
			bool m_problemChanged = true;

			Array<String> m_problemIds;

			std::unique_ptr<SivImGui::GUI> m_gui;

			const Font m_font;

			void refreshProblemLabels() {
				// 今あるラベルをすべて削除
				m_scrollViewReference->removeChildren();
				m_problemLabelReference.clear();

				//問題セレクタに要素を追加
				auto AddProblemSelectButton = [&](String problemId) -> SivImGui::Label* {
					auto label = std::make_unique<SivImGui::Label>();
					auto res = label.get();
					label->text = problemId;
					label->textColor = s3d::Palette::White;
					label->font = m_font;
					label->enableMouseOver = true;
					m_scrollViewReference->addChild(std::move(label));
					return res;
				};
				for (const String& problemId : m_problemIds) {
					m_problemLabelReference.push_back(AddProblemSelectButton(problemId));
				}
			}

		};

	}


	void Main() {

		EnsureResourceFileList();

		// フルスクリーン
		// シーンサイズ固定
		// Window::SetFullscreen(true);
		Scene::SetResizeMode(ResizeMode::Keep);
		Scene::Resize(FIXED_SCENE_WIDTH, FIXED_SCENE_HEIGHT);

		//背景色を白色に(スクロールバーが見えにくかったため)
		Scene::SetBackground(Palette::Black);

		HeaderGui headerGui;

		// Esc キーで終了できる

		auto g = Global::GetInstance();

		auto solverControlGui = std::make_shared<SivImGui::GUI>(std::make_unique<SolverControllerGui>());
		headerGui.setMainGui(GUI_ID_SOLVER_CONTROLLER, solverControlGui);
		auto solverOutputGui = std::make_shared<SivImGui::GUI>(std::make_unique<SolverOutputGui>());
		headerGui.setMainGui(GUI_ID_SOLVER_OUTPUT, solverOutputGui);
		auto matchManagerGui = std::make_shared<SivImGui::GUI>(std::make_unique<MatchServerManagerGui>());
		headerGui.setMainGui(GUI_ID_SERVER_SETTING, matchManagerGui);

		while (System::Update())
		{
			headerGui.setProblemIds(g->solverControl->getProblemIds().value_or(Array<String>{}));

			headerGui.layout(Size(FIXED_SCENE_WIDTH, FIXED_SCENE_HEIGHT));
			headerGui.update();
			headerGui.draw();
		}
	}

}

void Main()
{
	Procon33::Main();
}
