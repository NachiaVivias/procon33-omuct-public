#include "HTTP_communication.h"

namespace Procon33 {

	namespace HTTP {

		URL url = U"http://172.28.1.1/";

		TextReader reader{ U"token.txt" };
		String token = reader.readAll();

		Optional<MatchInfo> matchInfo;
		Optional<ProblemInfo> problemInfo;
		Optional<ChunksInfo> chunksInfo;
		Optional<AnswerRequestData> answerRequestData;
		Optional<AnswerResponceData> answerResponceData;
		Array<Wave> chunksWaves;
		Optional<Wave> srcWave;

		String statusLine;

		void SetUrl(String _url) {
			url = _url;
		}

		Optional<bool> SendTest() {
			JSON src;
			URL requestURL = url + U"" + U"?token=" + token;
			HashTable<String, String> header = {  };
			FilePath path = U"test.json";
			if (auto res = SimpleHTTP::Get(requestURL, header, path)) {
				statusLine = res.getStatusLine();
				if (res.getStatusCode() == HTTPStatusCode::OK) {
					return true;
				}
				return none;
			}
			else return none;
		}

		Optional<MatchInfo> GetMatchInfo() {
			JSON src;
			URL requestURL = url + U"match" + U"?token=" + token;
			HashTable<String, String> header = {  };
			FilePath path = U"match_info_sample.json";
			if (auto res = SimpleHTTP::Get(requestURL, header, path)) {
				statusLine = res.getStatusLine();
				if (res.getStatusCode() == HTTPStatusCode::OK) {
					src = JSON::Load(U"match_info_sample.json");
					MatchInfo buf;
					buf.problemsNum = src[U"problems"].get<int32>();
					buf.changePenality = src[U"penalty"].get<int32>();
					for (JSON f : src[U"bonus_factor"].arrayView()) {
						buf.bonusFactor.push_back(f.get<double>());
					}
					matchInfo = buf;
					return matchInfo;
				}
				return none;
			}
			else return none;
		}
		Optional<ProblemInfo> GetProblemInfo() {
			URL requestURL = url + U"problem";
			HashTable<String, String> header = { {U"procon-token",token} };
			FilePath path = U"problem_info_sample.json";
			if (auto res = SimpleHTTP::Get(requestURL, header, path)) {
				statusLine = res.getStatusLine();
				if (res.getStatusCode() == HTTPStatusCode::OK) {
					ProblemInfo buf;
					buf = buf.FromJson(JSON::Load(U"problem_info_sample.json"));
					problemInfo = buf;
					return problemInfo;
				}
				return none;
			}

			return none;
		}


		Optional<ChunksInfo> GetChunks(int n) {
			URL requestURL = url + U"problem/chunks?n=" + ToString(n);
			HashTable<String, String> header = { {U"procon-token",token} };
			FilePath path = U"chunks_sample.json";
			const std::string data = JSON
			{
			}.formatUTF8();

			if (auto res = SimpleHTTP::Post(requestURL, header, data.data(), data.size(), path)) {
				statusLine = res.getStatusLine();
				if (res.getStatusCode() == HTTPStatusCode::OK) {
					ChunksInfo buf;
					buf = ChunksInfo::fromJson(JSON::Load(path));
					chunksInfo = buf;
					return chunksInfo;
				}
				return none;
			}

			return none;
		} 
		
		Optional<Wave> GetData(String fileName) {
			URL requestURL = url + U"problem/chunks/" + fileName;
			HashTable<String, String> header = { {U"procon-token",token} };
			FilePath path = U"src_sample.wav";
			// Print << fileName;
			if (auto res = SimpleHTTP::Get(requestURL, header, path)) {
				if (res.getStatusCode() == HTTPStatusCode::OK) {
					Wave src;
					WAVEDecoder decoder;
					src = decoder.decode(U"src_sample.wav");
					chunksWaves.push_back(src);
					srcWave = src;
					return srcWave;
				}
				return none;
			}

			return none;
		}

		Optional<AnswerResponceData> SendAnswer(std::string data) {
			URL requestURL = url + U"problem";
			HashTable<String, String> header = { {U"procon-token",token},{U"Content-Type",U"application/json"} };
			FilePath path = U"respons_sample.json";
			if (auto res = SimpleHTTP::Post(requestURL, header, data.data(), data.size(), path)) {
				if (res.getStatusCode() == HTTPStatusCode::OK) {
					return AnswerResponceData::fromJson(JSON::Load(path));
				}
				return none;
			}

			return none;
		}


		void SampleGui::ShowObject(const JSON& value)
		{
			switch (value.getType())
			{
			case JSONValueType::Empty:
				Console << U"empty";
				break;
			case JSONValueType::Null:
				Console << U"null";
				break;
			case JSONValueType::Object:
				for (const auto& object : value)
				{
					Console << U"[" << object.key << U"]";
					ShowObject(object.value);
				}
				break;
			case JSONValueType::Array:
				for (const auto& element : value.arrayView())
				{
					ShowObject(element);
				}
				break;
			case JSONValueType::String:
				Console << value.getString();
				break;
			case JSONValueType::Number:
				Console << value.get<double>();
				break;
			case JSONValueType::Bool:
				Console << value.get<bool>();
				break;
			}
		}

		//テスト用のGUI
		void SampleGui::update() {

			if (SimpleGUI::Button(U"getmatchinfo", Vec2{ 0,0 })) {
				ClearPrint();
				if (GetMatchInfo()) {
					Print << U"OK";
				}
				else {
					Print << U"error";
				}
				JSON src = JSON::Load(U"match_info_sample.json");
				ShowObject(src);
			}
			if (SimpleGUI::Button(U"getprobleminfo", Vec2{ 0,50 })) {
				ClearPrint();
				GetProblemInfo();
				JSON src = JSON::Load(U"problem_info_sample.json");
				ShowObject(src);
			}
			SimpleGUI::TextBox(n, Vec2{ 200,100 });
			if (SimpleGUI::Button(U"getchunks", Vec2{ 0,100 })) {
				ClearPrint();
				std::string N = n.text.narrow();
				JSON src = JSON::Load(U"chunks_sample.json");
				ShowObject(src);

			}
			SimpleGUI::TextBox(reFileName, Vec2{ 200,150 });
			if (SimpleGUI::Button(U"getWaveFile", Vec2{ 0,150 })) {
				ClearPrint();
				//String res = GetData(chunksInfo.fileNames[0]);
				//String res = GetData(reFileName.text);
				//Print << res;
			}

			SimpleGUI::TextBox(answerId, Vec2{ 100,400 });
			SimpleGUI::TextBox(answers, Vec2{ 400,400 });

			if (SimpleGUI::Button(U"add", Vec2{ 400,450 })) {
				answer[U"answers"].push_back(answers.text);
			}
			if (SimpleGUI::Button(U"sendanswer", Vec2{ 100,500 })) {
				ClearPrint();
				answer[U"problem_id"] = answerId.text;
				//Print << res;
				ShowObject(answer);
				JSON src = JSON::Load(U"respons_sample.json");
				ShowObject(src);
			}

		}

		//GUI実装
		int HTTPGui::Px(int key) { return (Bx + key); };
		int HTTPGui::Py(int key) { return (By + key); };
		void HTTPGui::setBase(int Kx, int Ky) {
			Bx = Kx; By = Ky;
		}
		void HTTPGui::update() {
			if (SimpleGUI::Button(U"Get Match Info", Vec2(Px(0), Py(0)))){

			}
			if (SimpleGUI::Button(U"Get Problem Info", Vec2(Px(0), Py(50)))) {

			}
			if (SimpleGUI::Button(U"Get Chunks Info", Vec2(Px(0), Py(100)))) {

			}SimpleGUI::TextBox(n, Vec2{ Px(200),Py(100) });
			if (SimpleGUI::Button(U"Get wave data", Vec2(Px(0), Py(150)))) {

			}SimpleGUI::TextBox(FileName, Vec2{ Px(200),Py(150) });
		}


	} // namespace HTTP

} // namespace Procon33
