#pragma once
#include<Siv3D.hpp>

#include"data_format.h"

namespace Procon33 {

	namespace HTTP {

		extern URL url;
		extern String token;

		//高専プロコンサーバーにリクエストを送ってレスポンスの情報を返す
		//取得したデータは指定したpathに保存する

		void SetUrl(String _url);

		//MemoryWriterなるものに保存もできるっぽくて触ってみたがよくわからなかったのであきらめた
		void SetToken(String t);

		Optional<bool> SendTest();

		Optional<MatchInfo> GetMatchInfo();
		Optional<ProblemInfo> GetProblemInfo();


		Optional<ChunksInfo> GetChunks(int n);

		Optional<Wave> GetData(String fileName);

		Optional<AnswerResponceData> SendAnswer(std::string data);


		class SampleGui {
		public:
			TextEditState answerId;
			TextEditState answers;
			TextEditState reFileName;
			TextEditState to;
			TextEditState n;
			JSON answer;

			static void ShowObject(const JSON& value);

			void update();

		};

		class HTTPGui {
		private:
			int Bx = 0, By = 0;
		public:
			TextEditState answerId;
			TextEditState answers;
			TextEditState FileName;
			TextEditState to;
			TextEditState n;
			JSON answer;

			//実際に表示する座標を返す。
			int Px(int key);
			int Py(int key);

			void setBase(int Kx, int Ky);//基準点の座標の設定
			void update();
		};

	} // namespace HTTP

} // namespace Procon33
