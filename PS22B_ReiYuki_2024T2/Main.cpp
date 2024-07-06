# include <Siv3D.hpp>

// 敵の位置をランダムに作成する関数
Vec2 GenerateEnemy()
{
	return RandomVec2({ 50, 750 }, -20);
}

//アイテムの位置をランダムに作成する関数	
Vec2 GenerateItem()
{
	return RandomVec2({ 50, 750 }, -10);
}

void Main()
{
	Scene::SetBackground(ColorF{ 0.1, 0.2, 0.7 });

	const Font FONT{ FontMethod::MSDF, 48 };

	// 自機テクスチャ
	const Texture PLAYER_TEXTURE{ U"🚀"_emoji };
	// 敵テクスチャ
	const Texture ENEMY_TEXTURE{ U"👾"_emoji };
	//アイテムのテクスチャ
	const Texture UNRIVALED_ITEM_TEXTURE{ U"🌟"_emoji };

	// 自機
	Vec2 player_pos{ 400, 500 };
	constexpr double player_angle = 315_deg;
	// 敵
	Array<Vec2> enemies = { GenerateEnemy() };
	//アイテム
	Array<Vec2>items = { GenerateItem() };

	// 自機ショット
	Array<Vec2> player_bullets;
	// 敵ショット
	Array<Vec2> enemy_bullets;

	// 自機のスピード
	constexpr double player_speed = 550.0;
	// 自機ショットのスピード
	constexpr double player_bullet_speed = 500.0;
	// 敵のスピード
	constexpr double enemy_speed = 100.0;
	// 敵ショットのスピード
	constexpr double enemy_bullet_speed = 300.0;
	//アイテムのスピード
	constexpr double item_speed = 250.0;

	// 敵の発生間隔の初期値（秒）
	constexpr double initial_enemy_spawn_interval = 2.0;
	// 敵の発生間隔（秒）
	double enemy_spawn_time = initial_enemy_spawn_interval;
	// 敵の発生の蓄積時間（秒）
	double enemy_accumulated_time = 0.0;

	//アイテム発生間隔の初期値（秒）
	constexpr double initial_item_spawn_interval = 10.0;
	//アイテムの発生間隔（秒）
	double item_spawn_time = initial_item_spawn_interval;
	//アイテムの発生の蓄積時間（秒）
	double item_accumulated_time = 0.0;

	// 自機ショットのクールタイム（秒）
	constexpr double player_shot_cool_time = 0.3;
	// 自機ショットのクールタイムタイマー（秒）
	double player_shot_timer = 0.0;

	// 敵ショットのクールタイム（秒）
	constexpr double enemy_shot_cool_time = 0.9;
	// 敵ショットのクールタイムタイマー（秒）
	double enemy_shot_timer = 0.0;

	Effect effect;

	// ハイスコア
	int32 high_score = 0;
	// 現在のスコア
	int32 score = 0;

	while (System::Update())
	{
		// ゲームオーバー判定
		bool _isGameOver = false;

		const double delta_time = Scene::DeltaTime();
		enemy_accumulated_time += delta_time;
		item_accumulated_time += delta_time;
		player_shot_timer = Min((player_shot_timer + delta_time), player_shot_cool_time);
		enemy_shot_timer += delta_time;

		// 敵を発生させる
		while (enemy_spawn_time <= enemy_accumulated_time)
		{
			enemy_accumulated_time -= enemy_spawn_time;
			enemy_spawn_time = Max(enemy_spawn_time * 0.95, 0.3);
			enemies << GenerateEnemy();
		}

		//アイテムを生成する
		while (item_spawn_time <= item_accumulated_time)
		{
			item_accumulated_time -= item_spawn_time;
			item_spawn_time = Max(item_spawn_time * 0.95, 0.3);
			items << GenerateItem();
		}

		// アイテムを移動させる
		for (auto& item: items)
		{
			item.y += (delta_time * item_speed);
		}

		// 画面外に出たアイテムを削除する
		items.remove_if([](const Vec2& i) { return (700 < i.y); });

		// 自機の移動
		const Vec2 MOVE = Vec2{ (KeyRight.pressed() - KeyLeft.pressed()), 0 }
		.setLength(delta_time * player_speed * (KeyShift.pressed() ? 0.5 : 1.0));
		player_pos.moveBy(MOVE).clamp(Scene::Rect());

		// 自機ショットの発射
		if (MouseL.down())
		{
			player_bullets << player_pos.movedBy(0, -50);
		}

		// 自機ショットを移動させる
		for (auto& playerBullet : player_bullets)
		{
			playerBullet.y += (delta_time * -player_bullet_speed);
		}
		// 画面外に出た自機ショットを削除する
		player_bullets.remove_if([](const Vec2& b) { return (b.y < -40); });

		// 敵を移動させる
		for (auto& enemy : enemies)
		{
			enemy.y += (delta_time * enemy_speed);
		}
		// 画面外に出た敵を削除する
		enemies.remove_if([&](const Vec2& e)
		{
			if (700 < e.y)
			{
				// 敵が画面外に出たらゲームオーバー
				_isGameOver = true;
				return true;
			}
			else
			{
				return false;
			}
		});

		// 敵ショットの発射
		if (enemy_shot_cool_time <= enemy_shot_timer)
		{
			enemy_shot_timer -= enemy_shot_cool_time;

			for (const auto& enemy : enemies)
			{
				enemy_bullets << enemy;
			}
		}

		// 敵ショットを移動させる
		for (auto& enemy_bullet : enemy_bullets)
		{
			enemy_bullet.y += (delta_time * enemy_bullet_speed);
		}
		// 画面外に出た敵ショットを削除する
		enemy_bullets.remove_if([](const Vec2& b) {return (700 < b.y); });

		////////////////////////////////
		//
		//	攻撃判定
		//
		////////////////////////////////

		// 敵 vs 自機ショット
		for (auto it_enemy = enemies.begin(); it_enemy != enemies.end();)
		{
			const Circle ENEMY_CIRCLE{ *it_enemy, 40 };
			bool isSkip = false;

			for (auto it_bullet = player_bullets.begin(); it_bullet != player_bullets.end();)
			{
				if (ENEMY_CIRCLE.intersects(*it_bullet))
				{
					// 爆発エフェクトを追加する
					effect.add([pos = *it_enemy](double t)
					{
						const double t2 = ((0.5 - t) * 2.0);
						Circle{ pos, (10 + t * 280) }.drawFrame((20 * t2), AlphaF(t2 * 0.5));
						return (t < 0.5);
					});

					it_enemy = enemies.erase(it_enemy);
					player_bullets.erase(it_bullet);
					++score;
					isSkip = true;
					break;
				}

				++it_bullet;
			}

			if (isSkip)
			{
				continue;
			}

			++it_enemy;
		}

		// 敵ショット vs 自機
		for (const auto& enemy_bullet : enemy_bullets)
		{
			// 敵ショットが playerPos の 20 ピクセル以内に接近したら
			if (enemy_bullet.distanceFrom(player_pos) <= 20)
			{
				// ゲームオーバーにする
				_isGameOver = true;
				break;
			}
		}

		// ゲームオーバーならリセットする
		if (_isGameOver)
		{
			player_pos = Vec2{ 400, 500 };
			enemies.clear();
			player_bullets.clear();
			enemy_bullets.clear();
			enemy_spawn_time = initial_enemy_spawn_interval;
			item_spawn_time = initial_item_spawn_interval;
			high_score = Max(high_score, score);
			score = 0;
		}

		////////////////////////////////
		//
		//	描画
		//
		////////////////////////////////

		// 背景のアニメーションを描画する
		for (int32 i = 0; i < 12; ++i)
		{
			const double A = Periodic::Sine0_1(2s, Scene::Time() - (2.0 / 12 * i));
			Rect{ 0, (i * 50), 800, 50 }.draw(ColorF(1.0, A * 0.2));
		}

		// 自機を描画する
		PLAYER_TEXTURE.resized(80).rotated(player_angle).drawAt(player_pos);

		// 自機ショットを描画する
		for (const auto& player_bullet : player_bullets)
		{
			Circle{ player_bullet, 8 }.draw(Palette::Orange);
		}

		// 敵を描画する
		for (const auto& enemy : enemies)
		{
			ENEMY_TEXTURE.resized(60).drawAt(enemy);
		}

		// 敵ショットを描画する
		for (const auto& enemy_bullet : enemy_bullets)
		{
			Circle{ enemy_bullet, 4 }.draw(Palette::White);
		}

		//アイテムを描画する
		for (const auto& item : items)
		{
			UNRIVALED_ITEM_TEXTURE.resized(60).drawAt(item);
		}

		// 爆発エフェクトを描画する
		effect.update();

		// スコアを描画する
		FONT(U"{} [{}]"_fmt(score, high_score)).draw(30, Arg::bottomRight(780, 580));
	}
}
