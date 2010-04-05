TMPFILE="/tmp/construct_foobar"

construct()
{
    touch "${TMPFILE}" || mu_failure "Could not create ${TMPFILE}"
}

destruct()
{
    rm -f "${TMPFILE}" || mu_failure "Coult not remove ${TMPFILE}"
}

fixture_Shell_setup()
{
    HELLO_WORLD="hello world"
}

test_Shell_success()
{
    mu_success
}

test_Shell_failure()
{
    mu_expect failure
    mu_failure "I don't like this"
}

test_Shell_assert_success()
{
    mu_assert [ "foo" = "foo" ]
}

test_Shell_assert_failure()
{
    mu_expect assertion
    mu_assert [ "foo" = "bar" ]
}

test_Shell_success_subshell()
{
    (
	( mu_success )
	mu_failure "Should not be here"
    )
}

test_Shell_fixture_setup()
{
    mu_assert [ "${HELLO_WORLD}" = "hello world" ]
}

test_Shell_log()
{
    mu_warning "This is a warning"
    mu_info "This is informational"
    mu_verbose "This is verbose output"
    mu_debug "This is debugging information"
    mu_trace "This is trace information"
}

test_Shell_timeout()
{
    # Expect to time out
    mu_expect timeout
    # Time out in 100 milliseconds
    mu_timeout 100
    # Sleep for 1 second
    sleep 1
}

test_Shell_resource_section()
{
    mu_assert [ "$(mu_resource_from_section "global" "greeting")" = "Hello, world!" ]
}

test_Shell_resource_section_bogus()
{
    mu_expect resource
    mu_resource_from_section "abcd" "xyz"
}

test_Shell_resource()
{
    mu_assert [ "$(mu_resource "resource string")" = "42" ]
}

test_Shell_resource_bogus()
{
    mu_expect resource
    mu_resource "bad resource"
}

test_Shell_construct()
{
    mu_assert [ -f "${TMPFILE}" ]
}