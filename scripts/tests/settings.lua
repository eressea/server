require 'lunit'

module('tests.eressea.settings', package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
end

function test_settings()
    assert_equal(nil, eressea.settings.get('foo'))
    eressea.settings.set('foo', 'bar')
    assert_equal('bar', eressea.settings.get('foo'))
end
