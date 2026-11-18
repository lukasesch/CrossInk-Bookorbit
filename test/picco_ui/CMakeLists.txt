# PiccoUiTest builds the Picco UI value layer with the theme on, PiccoUiOffTest with it off.
foreach(variant IN ITEMS PiccoUiTest PiccoUiOffTest)
  add_executable(${variant} PiccoUiTest.cpp PiccoLayoutTest.cpp PiccoHomeTest.cpp PiccoLibrary4ATest.cpp PiccoLibrary4A2Test.cpp PiccoPagedListTest.cpp Picco4B2Test.cpp PiccoSystem4A3Test.cpp Picco4B3Test.cpp PiccoSheetTest.cpp PiccoP5Test.cpp Picco4R2Test.cpp)
  target_include_directories(${variant} PRIVATE
    ${REPO_ROOT}/include
    ${REPO_ROOT}/test/persistable_store_atomic/stubs
    ${REPO_ROOT}/src
    ${REPO_ROOT}/lib/Serialization
    ${REPO_ROOT}/lib/Epub
  )
  target_link_libraries(${variant} PRIVATE crosspoint_test_common GTest::gtest_main ArduinoJson)
  gtest_discover_tests(${variant})
endforeach()
target_compile_definitions(PiccoUiTest PRIVATE CROSSINK_APP_PICCO_UI=1 PICCO_UI_EXPECT_ON=1)
target_compile_definitions(PiccoUiOffTest PRIVATE CROSSINK_APP_PICCO_UI=0 PICCO_UI_EXPECT_ON=0)
