grep -n '#if' $(find $1 -type f | grep -v '\.git\|gcc') | grep -v '_H$'

