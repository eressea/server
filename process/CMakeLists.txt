install(PROGRAMS create-orders backup-* run-turn send-zip-report
send-bz2-report compress.py compress.sh epasswd.py
accept-orders.py getemail.py checkpasswd.py
sendreport.sh sendreports.sh orders-accept DESTINATION bin)

install(DIRECTORY cron/ DESTINATION bin USE_SOURCE_PERMISSIONS
FILES_MATCHING PATTERN "*.cron")

install(DIRECTORY procmail DESTINATION share)
