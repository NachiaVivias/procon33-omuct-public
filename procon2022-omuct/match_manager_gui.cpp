#include "match_manager_gui.h"
#include "HTTP_communication.h"
#include "solver_controller.h"
#include "global.h"


namespace Procon33 {

	namespace {

		const String DEFALT_SERVER_ID = U"プロコンサーバー";
		const String VIRTUAL_SERVER_ID_PREFIX = U"疑似サーバー";
		const String GUI_ID_RESET_MATCH = U"サーバー切り替え";
		const String GUI_ID_EDIT_URL = U"URL編集";

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


	MatchServerManagerGui::MatchServerManagerGui()
		: m_matchManager(Global::GetInstance()->matchManager)
		, m_currentMatchId(DEFALT_SERVER_ID)
		, m_nextMatchId(DEFALT_SERVER_ID)
		, m_urlEditorState(std::make_unique<TextEditState>(U"http://172.28.1.1/"))
		, m_editingUrl(false)
	{
	}

	void MatchServerManagerGui::update(Rect rect) {
		auto g = Global::GetInstance();
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

		auto AddServerSelectButton = [&](String serverId, String text, Optional<double> width = unspecified) -> bool {
			bool res = AddButton(serverId, text, width);
			Color highlightColor = GUI_COLOR_OFF;
			if (m_nextMatchId == serverId) highlightColor = GUI_COLOR_HOLD;
			if (m_currentMatchId == serverId) highlightColor = GUI_COLOR_ON;
			m_highlightTime[serverId].duration = 1.0;
			m_highlightTime[serverId].decrease = 0.0;
			m_highlightTime[serverId].color = highlightColor;
			if (res) m_nextMatchId = serverId;
			return res;
		};

		auto AddLabel = [&](String text, double height, Optional<double> minWidth, Optional<double> maxWidth, Color) -> RectF {
			double width = SimpleGUI::GetFont()(text).region(height * 0.8).w + height * 0.4;
			double scale = 1.0;
			if (minWidth.has_value()) width = Max(*minWidth, width);
			if (maxWidth.has_value()) { scale = *maxWidth / width;  width = Min(*maxWidth, width); }
			RectF rect = RectF(XLine + GUI_XDISTANCE * 0.5, YLine - height * 0.5, width, height);
			XLine += GUI_XDISTANCE + width;
			return rect;
		};

		bool serverUpdated = false;

		NewLine(GUI_BASE_HEIGHT);
		AddServerSelectButton(DEFALT_SERVER_ID, DEFALT_SERVER_ID);
		NewLine(GUI_BASE_HEIGHT);
		AddLabel(U"URL", 40.0, unspecified, unspecified, GUI_COLOR_TRANSPARENT);
		XLine += 400.0; // textbox
		String tx = m_editingUrl ? U"確定" : U"編集";
		if (AddButton(GUI_ID_EDIT_URL, tx)) {
			if (m_editingUrl) {
				HTTP::SetUrl(m_urlEditorState->text);
			}
			m_editingUrl = !m_editingUrl;
		}

		NewLine(GUI_BASE_HEIGHT);
		AddLabel(U"試合サンプル", 40.0, unspecified, unspecified, GUI_COLOR_TRANSPARENT);

		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 0; i < 8; i++) AddServerSelectButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);
		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 8; i < 16; i++) AddServerSelectButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);
		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 16; i < 18; i++) AddServerSelectButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);
		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 18; i < 20; i++) AddServerSelectButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);

		NewLine(GUI_BASE_HEIGHT);
		NewLine(GUI_BASE_HEIGHT);
		if (AddButton(GUI_ID_RESET_MATCH, GUI_ID_RESET_MATCH)) {
			bool res = m_highlightTime[GUI_ID_RESET_MATCH].duration >= 0.7;
			if (res) {
				m_currentMatchId = m_nextMatchId;
				serverUpdated = true;
			}
			m_highlightTime[GUI_ID_RESET_MATCH].duration = 1.0;
			m_highlightTime[GUI_ID_RESET_MATCH].decrease = 1.0 / HIGHLINGHT_DURATION;
			m_highlightTime[GUI_ID_RESET_MATCH].color = (res ? GUI_COLOR_GOOD : GUI_COLOR_BAD);
		}

		if (serverUpdated) {
			if (m_currentMatchId == DEFALT_SERVER_ID) {
				m_matchManager->m_server = AbstractServer::GetRealServerInterface();
			}
			else if(m_currentMatchId.starts_with(VIRTUAL_SERVER_ID_PREFIX)) {
				int32 index = Parse<int32>(m_currentMatchId.substr(VIRTUAL_SERVER_ID_PREFIX.size()));
				m_matchManager->m_server = AbstractServer::GetVirtualMatchServerById(index);
			}
			g->solverControl->setServer(g->matchManager->getServer());
		}

		m_matchManager->m_flagMatchReset = serverUpdated;
	}

	void MatchServerManagerGui::draw(Rect rect) const
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

		auto AddLabel = [&](String text, double height, Optional<double> minWidth, Optional<double> maxWidth, Color color) -> RectF {
			double width = SimpleGUI::GetFont()(text).region(height * 0.8).w + height * 0.4;
			double scale = 1.0;
			if (minWidth.has_value()) width = Max(*minWidth, width);
			if (maxWidth.has_value()) { scale = *maxWidth / width;  width = Min(*maxWidth, width); }
			RectF rect = RectF(XLine + GUI_XDISTANCE * 0.5, YLine - height * 0.5, width, height);
			rect.draw(color);
			XLine += GUI_XDISTANCE + width;
			SimpleGUI::GetFont()(text).drawAt(scale * height * 0.8, rect.center());
			return rect;
		};

		NewLine(GUI_BASE_HEIGHT);
		AddButton(DEFALT_SERVER_ID, DEFALT_SERVER_ID);
		NewLine(GUI_BASE_HEIGHT);
		AddLabel(U"URL", 40.0, unspecified, unspecified, GUI_COLOR_TRANSPARENT);
		SimpleGUI::TextBoxAt(*m_urlEditorState, Vec2(XLine + 200.0, YLine), 400.0, unspecified, m_editingUrl);
		XLine += 400.0;
		String tx = m_editingUrl ? U"確定" : U"編集";
		AddButton(GUI_ID_EDIT_URL, tx);

		NewLine(GUI_BASE_HEIGHT);
		AddLabel(U"試合サンプル", 40.0, unspecified, unspecified, GUI_COLOR_TRANSPARENT);

		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 0; i < 8; i++) AddButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);
		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 8; i < 16; i++) AddButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);
		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 16; i < 18; i++) AddButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);
		NewLine(GUI_BASE_HEIGHT);
		for (int32 i = 18; i < 20; i++) AddButton(VIRTUAL_SERVER_ID_PREFIX + ToString(i), ToString(i), 60.0);

		NewLine(GUI_BASE_HEIGHT);
		NewLine(GUI_BASE_HEIGHT);
		AddButton(GUI_ID_RESET_MATCH, GUI_ID_RESET_MATCH);
		AddLabel(U"（ダブルクリック）", 40.0, unspecified, unspecified, ColorF(Palette::Black, 0.0));
	}

}
