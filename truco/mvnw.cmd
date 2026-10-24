@echo off
set MAVEN_HOME=%USERPROFILE%\tools\maven\apache-maven-3.9.6
set JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-25.0.2.10-hotspot
set PATH=%MAVEN_HOME%\bin;%JAVA_HOME%\bin;%PATH%
mvn %*
