cmake_minimum_required (VERSION 3.12)
project (doc LANGUAGES C)

# TYPE MAN is insufficient to get it into the correct subdirectory
install(FILES lp5250d.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES scs2ascii.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES scs2ps.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES scs2pdf.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES tn5250.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES tn5250rc.5 DESTINATION ${CMAKE_INSTALL_MANDIR}/man5)
