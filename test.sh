set -x

./zvonilka listen &

./zvonilka call 127.0.0.1

kill $!
