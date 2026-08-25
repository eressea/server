print("LUA_PATH is " .. os.getenv('LUA_PATH'))
print("LUA_CPATH is " .. os.getenv('LUA_CPATH'))
print("PACKAGE.PATH is " .. package.path)
lunit = require('lunit')

module('tests.example', lunit.testcase, package.seeall)
function test_example()
    assert_true(true)
end

result = lunit.main()
return result.errors + result.failed

