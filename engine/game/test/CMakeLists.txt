find_package(Corrade REQUIRED TestSuite)

corrade_add_test(GameFixedStepperTest FixedStepperTest.cxx LIBRARIES Vibe::GameCore)
corrade_add_test(GameActionMapTest ActionMapTest.cxx LIBRARIES Vibe::GameCore)
corrade_add_test(GameInputSystemTest InputSystemTest.cxx LIBRARIES Vibe::GameCore)
corrade_add_test(GameLobbyTest LobbyTest.cxx LIBRARIES Vibe::GameCore)
corrade_add_test(GameViewLayoutTest ViewLayoutTest.cxx LIBRARIES Vibe::GameCore)
corrade_add_test(GameInputScriptTest InputScriptTest.cxx LIBRARIES Vibe::GameCore)
corrade_add_test(GameListTest GameListTest.cxx LIBRARIES Vibe::GameCore)
