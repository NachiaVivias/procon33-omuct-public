#pragma once

#include <memory>
#include "../stdafx.h"
#include "../wave_int32.h"
#include "../data_format.h"


namespace Procon33 {

	// getter 系以外で bool を返すメンバ関数が false を返す場合はエラーが発生しているので、
	// getLastErrorMessage でエラーメッセージを取得してください。
	// 進行度を起動すると書いてあってもしないかもしれませんし、書いてない ID の進行度が起動するかもしれません。
	class SolverInterface {
	public:

		static const String PROGRESS_ID_PRECALC;
		static const String PROGRESS_ID_STOP;

		// 仮想クラス（ポリモーフィズム）のお約束
		~SolverInterface() = default;

		// 最後に発生したエラーのメッセージを取得。
		virtual String getLastErrorMessage();

		// targetTags は "J" または "E" を含む。
		// targetTags で指定された言語の読みデータをディレクトリ targetDir から検索し、そこから前計算を起動する。
		// 進行度 Solver Precalc を起動。
		// この関数の処理の後、ソルバーは一旦停止するかもしれない。
		virtual bool precalc(StringView targetDir, Array<String> targetTags) = 0;

		// 現在の試合に問題の情報を追加し、解く問題を変更する。
		// 同じ ID の情報で繰り返し呼ぶと、上書きする。
		virtual bool setProblem(const ProblemInfo& problemInfo) = 0;
		virtual bool setProblem(String problemId) = 0;

		// 与えられる波形を追加。
		// 波形の位置関係を表す番号がファイル名に振られているので chunkIndex に与える。
		// chunkIndex は 0 から数え始める。
		virtual bool addChunk(StringView problemId, int32 chunkIndex, WaveInt32 wave) = 0;

		// true を与えるとソルバーを起動する。
		// false を与えるとソルバーを止める。
		// 進行度 Solver Stop を起動。
		virtual bool runOrStop(bool run) = 0;

		// メインの計算に使用するスレッドの個数を設定する。
		// （たぶん）メモリが律速なので threadCount を上げても高速化しないかも
		virtual bool setThreadCount(int32 threadCount) = 0;

		// globalRect に収まるように、情報を描画。
		virtual void draw(RectF globalRect = RectF(100.0, 100.0, 800.0, 500.0)) = 0;

		// メインの処理が実行中かどうか
		virtual bool isRunning() = 0;

		// 非同期処理の進行度を 0.0 から 1.0 で表す。
		// "Solver_" から始まる ID と、進行度の 値を組にして返す。
		virtual Array<std::pair<String, double>> getProgress() = 0;

		// 答えを取得
		virtual AnswerRequestData getAnswer() = 0;

	protected:

		String m_lastError;

	};

} // namespace Procon33


namespace Procon33 {

	std::shared_ptr<SolverInterface> GetSolverInstance();

} // namespace Procon33

