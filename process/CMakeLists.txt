install(PROGRAMS create-orders backup-eressea run-turn send-zip-report
send-bz2-report compress.py compress.sh epasswd.py orders-process
checkpasswd.py sendreport.sh sendreports.sh orders-accept functions.sh DESTINATION bin)

install(DIRECTORY cron/ DESTINATION bin USE_SOURCE_PERMISSIONS
FILES_MATCHING PATTERN "*.cron")
install(DIRECTORY cron/ DESTINATION etc USE_SOURCE_PERMISSIONS
FILES_MATCHING PATTERN "crontab.template")

install(DIRECTORY procmail DESTINATION share)
