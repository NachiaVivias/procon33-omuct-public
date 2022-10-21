#pragma once
#include "stdafx.h"
#include "global.h"
#include "problem_generator/match_virtual_server.h"

namespace Procon33 {

	class MatchServerManagerGui;

	class MatchServerManager {
	public:

		MatchServerManager() : m_server(AbstractServer::GetRealServerInterface()) {}

		std::shared_ptr<AbstractServer> getServer() { return m_server; }
		bool isMatchUpdated() { return m_flagMatchReset; }

	private:
		friend MatchServerManagerGui;

		std::shared_ptr<AbstractServer> m_server;
		bool m_flagMatchReset = false;

		void update() { m_flagMatchReset = false; }
	};

	class MatchServerManagerGui :public SivImGui::Widget {
	public:

		MatchServerManagerGui();

	private:

		struct HighlightState {
			double duration;
			double decrease;
			Color color;
		};

		int32 HIGHLINGHT_DURATION = 100;
		std::map<String, HighlightState> m_highlightTime;
		String m_currentMatchId;
		String m_nextMatchId;

		std::unique_ptr<TextEditState> m_urlEditorState;
		bool m_editingUrl;

		std::shared_ptr<MatchServerManager> m_matchManager;

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
