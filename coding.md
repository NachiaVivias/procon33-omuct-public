# procon2022-omuct コーディングルール

Siv3D を参考にします。ただし、複雑さを抑えるため、省略する部分があります。

## 命名規則

リポジトリ名は `chain-case`

ファイル名は `snake_case`

新しく定義する型名、名前空間名は `UpperCamelCase`

コンパイル時定数 (static const または constexpr) は `UPPER_SNAKE_CASE`

変数名は `lowerCamelCase`

関数名は `lowerCamelCase`

class は

- メンバ変数のアクセスは public 以外にする
- メンバ変数名の先頭に `m_` をつける

struct は

- メンバ変数のアクセスは public にする
- メンバ変数名は普通の変数名と同じ法則

## その他

ヘッダーファイルの拡張子は `.h`
