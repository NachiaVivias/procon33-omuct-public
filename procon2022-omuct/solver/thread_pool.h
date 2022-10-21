#pragma once

// 参考 ： (基礎) https://qiita.com/termoshtt/items/c01745ea4bcc89d37edc
//         (主に理論) https://contentsviewer.work/Master/Cpp/how-to-implement-a-thread-pool/article
//         (コード例) https://zenn.dev/rita0222/articles/13953a5dfb9698

#include "../stdafx.h"
#include <functional>

namespace Procon33 {

	class ThreadPool {
	public:

		using Task = std::function<void(uint32)>;

		// コピーとかだめだよ
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool(const ThreadPool&) = delete;

		// 仮想クラスのお約束
		// デストラクト時には sync が呼ばれる
		virtual ~ThreadPool() = default;

		// スレッドを threadCount 個管理するプールを作成
		static std::unique_ptr<ThreadPool> Construct(uint32 threadCount);

		// ハードウェアでサポートされるスレッド数を返す。
		// わからないなら none を返す。
		static Optional<uint32> HardwareConcurrency() {
			uint32 res = std::thread::hardware_concurrency();
			if (res == 0) return none;
			return res;
		}

		// タスクをキューに追加する
		virtual void pushTask(Task&& task) = 0;

		// キューにあるタスクをすべて処理する
		virtual void sync() = 0;

	protected:

		// static 関数 Construct を使うこと
		ThreadPool();

	};

} // namespace Procon33
