Set-Location -Path searchclient_java

# Build the project
javac searchclient/*.java
jar cfm searchclient.jar MANIFEST.MF searchclient/*.class