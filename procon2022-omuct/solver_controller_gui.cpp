#include "solver_controller_gui.h"
#include "resource_check.h"

namespace Procon33 {

	namespace {

		const String GUI_ID_MATCH = U"試合";
		const String GUI_ID_PROBLEM = U"問題";
		const String GUI_ID_CHUNK = U"断片";
		const String GUI_ID_PRECALC1 = U"前計算1";
		const String GUI_ID_PRECALC2 = U"前計算2";
		const String GUI_ID_PRECALC3 = U"前計算3";
		const String GUI_ID_SOLVERON = U"ON";
		const String GUI_ID_SOLVEROFF = U"OFF";
		const String GUI_ID_ANSWER = U"回答";
		const String GUI_ID_RESET_MATCH = U"試合リセット";

		const double GUI_BASE_HEIGHT = 52.0;
		const double GUI_HIGHLIGHT_WIDTH = 4.0;
		const double GUI_XDISTANCE = 10.0;
		const double GUI_PADDING_OVER = 15.0;

		const Color GUI_COLOR_TRANSPARENT = Palette::Gray.withAlpha(0);
		const Color GUI_COLOR_GOOD = Color(0, 180, 255);
		const Color GUI_COLOR_WARNING = Palette::Yellow;
		const Color GUI_COLOR_BAD = Palette::Red;
		const Color GUI_COLOR_ON = GUI_COLOR_GOOD;
		const Color GUI_COLOR_HOLD = GUI_COLOR_WARNING;
		const Color GUI_COLOR_OFF = Palette::Gray.withAlpha(0);

	}

	void SolverControllerGui::update(Rect rect) {
		for (auto& a : m_highlightTime) a.second.duration = Max(a.second.duration - a.second.decrease, 0.0);

		double YBase = rect.y + GUI_PADDING_OVER;
		double XBase = rect.x + GUI_PADDING_OVER;
		double YLine = YBase;
		double XLine = XBase;
		double currentHeight = 0.0;

		auto NewLine = [&](double height) {
			XLine = XBase;
			YLine = YLine + (height + currentHeight) / 2.0;
			currentHeight = height;
		};

		auto NewColumn = [&](double slideWidth) {
			XBase += slideWidth;
			YLine = YBase;
			XLine = XBase;
			currentHeight = 0.0;
		};

		auto Tab = [&](double posX) {
			XLine = XBase + posX;
		};

		auto AddButton = [&](String guiid, String text, Optional<double> width = unspecified) -> bool {
			auto buttonRect = SimpleGUI::ButtonRegionAt(text, Vec2(XLine, YLine), width);
			double Xmiddle = XLine + (GUI_XDISTANCE + buttonRect.w) * 0.5;
			buttonRect = SimpleGUI::ButtonRegionAt(text, Vec2(Xmiddle, YLine), width);
			XLine += GUI_XDISTANCE + buttonRect.w;
			return buttonRect.contains(Cursor::PosF()) && MouseL.down();
		};

		if (m_controller.get()) {
			NewLine(GUI_BASE_HEIGHT);
			if (AddButton(GUI_ID_MATCH, GUI_ID_MATCH)) {
				bool res = m_controller->requestMatchInfo();
				m_highlightTime[GUI_ID_MATCH].duration = 1.0;
				m_highlightTime[GUI_ID_MATCH].decrease = 1.0 / HIGHLINGHT_DURATION;
				m_highlightTime[GUI_ID_MATCH].color = (res ? GUI_COLOR_GOOD : GUI_COLOR_BAD);
			}

			NewLine(GUI_BASE_HEIGHT);
			if (AddButton(GUI_ID_PROBLEM, GUI_ID_PROBLEM)) {
				bool res = m_controller->requestProblemInfo();
				if (res) m_controller->setCurrentProblem(m_controller->getProblemIds().value().back());
				m_highlightTime[GUI_ID_PROBLEM].duration = 1.0;
				m_highlightTime[GUI_ID_PROBLEM].decrease = 1.0 / HIGHLINGHT_DURATION;
				m_highlightTime[GUI_ID_PROBLEM].color = (res ? GUI_COLOR_GOOD : GUI_COLOR_BAD);
			}

			NewLine(GUI_BASE_HEIGHT);
			if (auto p = m_controller->getCurrentProblemState(); p.has_value()) {
				if (AddButton(GUI_ID_CHUNK, GUI_ID_CHUNK)) {
					bool res = m_controller->requestAnotherChunk();
					m_highlightTime[GUI_ID_CHUNK].duration = 1.0;
					m_highlightTime[GUI_ID_CHUNK].decrease = 1.0 / HIGHLINGHT_DURATION;
					m_highlightTime[GUI_ID_CHUNK].color = (res ? GUI_COLOR_GOOD : GUI_COLOR_BAD);
				}
			}

			NewLine(GUI_BASE_HEIGHT);
			if (AddButton(GUI_ID_SOLVERON, GUI_ID_SOLVERON)) m_controller->turnOnSolver();
			m_highlightTime[GUI_ID_SOLVERON].duration = 1.0;
			m_highlightTime[GUI_ID_SOLVERON].decrease = 0.0;
			m_highlightTime[GUI_ID_SOLVERON].color = (m_controller->getSolver()->isRunning() ? GUI_COLOR_ON : GUI_COLOR_OFF);
			if (AddButton(GUI_ID_SOLVEROFF, GUI_ID_SOLVEROFF)) m_controller->turnOffSolver();
			m_highlightTime[GUI_ID_SOLVEROFF].duration = 1.0;
			m_highlightTime[GUI_ID_SOLVEROFF].decrease = 0.0;
			m_highlightTime[GUI_ID_SOLVEROFF].color = (m_controller->getSolver()->isRunning() ? GUI_COLOR_OFF : GUI_COLOR_ON);

			NewLine(GUI_BASE_HEIGHT);
			if (AddButton(GUI_ID_PRECALC1, U"J")) if (m_controller->precalc({ U"J" })) m_precalcId = GUI_ID_PRECALC1;
			if (AddButton(GUI_ID_PRECALC2, U"E")) if (m_controller->precalc({ U"E" })) m_precalcId = GUI_ID_PRECALC2;
			if (AddButton(GUI_ID_PRECALC3, U"J+E")) if (m_controller->precalc({ U"J", U"E"})) m_precalcId = GUI_ID_PRECALC3;
			for (const String& guiid : { GUI_ID_PRECALC1, GUI_ID_PRECALC2, GUI_ID_PRECALC3 }) {
				m_highlightTime[guiid].duration = 1.0;
				m_highlightTime[guiid].decrease = 0.0;
				m_highlightTime[guiid].color = ((m_precalcId == guiid) ? GUI_COLOR_ON : GUI_COLOR_OFF);
			}

			NewLine(GUI_BASE_HEIGHT);
			if (AddButton(GUI_ID_ANSWER, GUI_ID_ANSWER)) {
				auto res = m_controller->sendTheAnswer();
				m_highlightTime[GUI_ID_ANSWER].duration = 1.0;
				m_highlightTime[GUI_ID_ANSWER].decrease = 1.0 / HIGHLINGHT_DURATION;
				m_highlightTime[GUI_ID_ANSWER].color = (res ? GUI_COLOR_GOOD : GUI_COLOR_BAD);
			}

			NewLine(GUI_BASE_HEIGHT * 4);
			NewLine(GUI_BASE_HEIGHT);
			if (AddButton(GUI_ID_RESET_MATCH, GUI_ID_RESET_MATCH)) {
				bool res = m_highlightTime[GUI_ID_RESET_MATCH].duration >= 0.7;
				if (res) m_controller->resetProblems();
				m_highlightTime[GUI_ID_RESET_MATCH].duration = 1.0;
				m_highlightTime[GUI_ID_RESET_MATCH].decrease = 1.0 / HIGHLINGHT_DURATION;
				m_highlightTime[GUI_ID_RESET_MATCH].color = (res ? GUI_COLOR_GOOD : GUI_COLOR_BAD);
			}
		}
	}

	void SolverControllerGui::draw(Rect rect) const
	{
		double YBase = rect.y + GUI_PADDING_OVER;
		double XBase = rect.x + GUI_PADDING_OVER;
		double YLine = YBase;
		double XLine = XBase;
		double currentHeight = 0.0;

		auto NewLine = [&](double height) {
			XLine = XBase;
			YLine = YLine + (height + currentHeight) / 2.0;
			currentHeight = height;
		};

		auto NewColumn = [&](double slideWidth) {
			XBase += slideWidth;
			YLine = YBase;
			XLine = XBase;
			currentHeight = 0.0;
		};

		auto Tab = [&](double posX) {
			XLine = XBase + posX;
		};

		auto AddButton = [&](String guiid, String text, Optional<double> width = unspecified) -> RectF {
			auto buttonRect = SimpleGUI::ButtonRegionAt(text, Vec2(XLine, YLine), width);
			double Xmiddle = XLine + (GUI_XDISTANCE + buttonRect.w) * 0.5;
			buttonRect = SimpleGUI::ButtonRegionAt(text, Vec2(Xmiddle, YLine), width);

			RectF rect1 = buttonRect.stretched(GUI_HIGHLIGHT_WIDTH);
			if (m_highlightTime.count(guiid)) {
				auto info = m_highlightTime.at(guiid);
				double alpha = (double)info.color.a * info.duration;
				RoundRect(rect1, GUI_HIGHLIGHT_WIDTH).draw(info.color.withAlpha((uint32)alpha));
			}

			[[maybe_unused]] bool x = SimpleGUI::ButtonAt(text, buttonRect.center(), width);
			XLine += GUI_XDISTANCE + buttonRect.w;
			return buttonRect;
		};

		auto AddLabel = [&](String text, double height, Optional<double> minWidth, Optional<double> maxWidth, Color color, Color fontColor = Palette::White) -> RectF {
			double width = SimpleGUI::GetFont()(text).region(height * 0.8).w + height * 0.4;
			double scale = 1.0;
			if (minWidth.has_value()) width = Max(*minWidth, width);
			if (maxWidth.has_value()) { scale = Min(1.0, *maxWidth / width);  width = Min(*maxWidth, width); }
			RectF rect = RectF(XLine + GUI_XDISTANCE * 0.5, YLine - height * 0.5, width, height);
			rect.draw(color);
			XLine += GUI_XDISTANCE + width;
			SimpleGUI::GetFont()(text).drawAt(scale * height * 0.8, rect.center(), fontColor);
			return rect;
		};

		if (m_controller.get()) {
			NewLine(GUI_BASE_HEIGHT);
			AddButton(GUI_ID_MATCH, GUI_ID_MATCH);
			NewLine(GUI_BASE_HEIGHT);
			AddButton(GUI_ID_PROBLEM, GUI_ID_PROBLEM);
			NewLine(GUI_BASE_HEIGHT);
			if (auto p = m_controller->getCurrentProblemState(); p.has_value()) {
				AddButton(GUI_ID_CHUNK, GUI_ID_CHUNK);
				Array<Color> colorBuf(p->info.chunksNum, GUI_COLOR_BAD);
				for (auto& q : p->chunks) { colorBuf[q.index] = GUI_COLOR_GOOD; }
				for (size_t i = 0; i < colorBuf.size(); i++) AddLabel(ToString(i + 1), 40.0, 40.0, 40.0, colorBuf[i]);
			}
			NewLine(GUI_BASE_HEIGHT);
			AddButton(GUI_ID_SOLVERON, GUI_ID_SOLVERON);
			AddButton(GUI_ID_SOLVEROFF, GUI_ID_SOLVEROFF);

			NewLine(GUI_BASE_HEIGHT);
			AddButton(GUI_ID_PRECALC1, U"J");
			AddButton(GUI_ID_PRECALC2, U"E");
			AddButton(GUI_ID_PRECALC3, U"J+E");

			NewLine(GUI_BASE_HEIGHT);
			AddButton(GUI_ID_ANSWER, GUI_ID_ANSWER);

			NewLine(GUI_BASE_HEIGHT * 3);
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(m_controller->getLastError(), 25.0, unspecified, unspecified, GUI_COLOR_TRANSPARENT);
			NewLine(GUI_BASE_HEIGHT);
			AddButton(GUI_ID_RESET_MATCH, GUI_ID_RESET_MATCH);
			AddLabel(U"（ダブルクリック）", 40.0, unspecified, unspecified, GUI_COLOR_TRANSPARENT);

			NewColumn(260.0);

			const double INFO_TAB1 = 200.0;
			const double INFO_LABEL_H = 40.0;

			auto matchInfo = m_controller->getMatchInfo();
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(U"Bonus : ", INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT); Tab(INFO_TAB1);
			if (matchInfo.has_value()) {
				String tmp;
				for (size_t i = 0; i < matchInfo->bonusFactor.size(); i++) {
					if (i != 0) tmp += U"-";
					double val = matchInfo->bonusFactor[i];
					int32 decint = (int32)(Round(val * 100.0));
					tmp += ToString(decint / 100) + U"." + ToString(decint / 10 % 10) + ToString(decint % 10);
				}
				AddLabel(tmp, INFO_LABEL_H, unspecified, 430.0, GUI_COLOR_TRANSPARENT);
			}
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(U"Penalty : ", INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT); Tab(INFO_TAB1);
			if (matchInfo.has_value()) {
				int32 val = matchInfo->changePenality;
				String tmp = ToString(val);
				AddLabel(tmp, INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT);
			}
			NewLine(GUI_BASE_HEIGHT);

			auto problemState = m_controller->getCurrentProblemState();
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(U"問題 ID : ", INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT); Tab(INFO_TAB1);
			if (problemState.has_value()) {
				AddLabel(problemState->info.problemId, INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT);
			}
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(U"断片数 : ", INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT); Tab(INFO_TAB1);
			if (problemState.has_value()) {
				AddLabel(ToString(problemState->info.chunksNum), INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT);
			}
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(U"重合せ数 : ", INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT); Tab(INFO_TAB1);
			if (problemState.has_value()) {
				AddLabel(ToString(problemState->info.dataNum), INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT);
			}
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(U"終了まで : ", INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT); Tab(INFO_TAB1);
			if (problemState.has_value()) {
				uint64 nows = Time::GetSecSinceEpoch();
				uint64 endtime = problemState->info.startTime + problemState->info.timeLimit;
				Color c = Palette::White;
				if (endtime <= nows) { nows = endtime; c = Palette::Yellow; }
				DateTime t2 = DateTime(2000, 1, 1) + Seconds(endtime - nows);
				AddLabel(t2.format(U"HH:mm:ss"), INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT, c);
			}
			NewLine(GUI_BASE_HEIGHT);
			AddLabel(U"最終提出 : ", INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT); Tab(INFO_TAB1);
			if (problemState.has_value()) if(problemState->answer.has_value()) {
				uint64 nows = problemState->answer->acceptTime;
				DateTime t = DateTime(1970, 1, 1) + Hours(9) + Seconds(nows);
				AddLabel(t.format(U"HH:mm:ss"), INFO_LABEL_H, unspecified, unspecified, GUI_COLOR_TRANSPARENT, Palette::Yellow);
			}
		}
	}




	void SolverControllerGui::MainFunctionUnitTest() {

		EnsureResourceFileList();

		auto solver = Global::GetInstance()->solver;
		auto solverController = Global::GetInstance()->solverControl;
		auto widget = std::make_unique<SolverControllerGui>();
		auto gui = SivImGui::GUI(std::move(widget));

		// フルスクリーン
		// シーンサイズ固定
		// Window::SetFullscreen(true);
		Scene::SetResizeMode(ResizeMode::Keep);
		Scene::Resize(1280, 720);

		// Esc キーで終了できる

		while (System::Update())
		{
			gui.update(Point(400, 100));
			gui.draw(Point(400, 100));

			ClearPrint();
			Print << U"時刻 ： " << Time::GetSecSinceEpoch();
			Print << U"試合情報：";
			auto matchInfo = solverController->getMatchInfo();
			if (matchInfo.has_value()) {
				Print << U"    bonusFactor : " << matchInfo->bonusFactor;
				Print << U"    changePenality : " << matchInfo->changePenality;
				Print << U"    problemsNum : " << matchInfo->problemsNum;
			}
			else Print << U"EMPTY";

			Print << U"問題情報： ";
			auto problemIds = solverController->getProblemIds();
			if (problemIds.has_value()) {
				for (auto problemId : *problemIds) {
					Print << U"    {";
					Print << U"    id = " << problemId;
					auto problemInfo = solverController->getProblemState(problemId);
					if (problemInfo.has_value()) {
						Print << U"        chunksNum : " << problemInfo->info.chunksNum;
						Print << U"        dataNum : " << problemInfo->info.dataNum;
						Print << U"        problemId : " << problemInfo->info.problemId;
						Print << U"        startTime : " << problemInfo->info.startTime;
						Print << U"        timeLimit : " << problemInfo->info.timeLimit;
					}
					else Print << U"    EMPTY";
					Print << U"    }";
				}
			}
			else Print << U"EMPTY";

			Print << U"-----------------------------";

			Print << solverController->getLastError();

			solver->draw(RectF(600.0, 400.0, 600.0, 300.0));
		}
	}

}
