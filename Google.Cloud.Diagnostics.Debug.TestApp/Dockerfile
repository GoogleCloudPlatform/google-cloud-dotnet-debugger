FROM gcr.io/csharp-debugger-eap/aspnetcore:1.1.4-2017-11-07_11_15
COPY . /app
WORKDIR /app
EXPOSE 8080
ENV ASPNETCORE_URLS=http://*:8080
#ENTRYPOINT ["ls", "-l", "/app/bin/Debug/netcoreapp1.1/publish"]
#ENTRYPOINT ["dotnet --info"]
#ENTRYPOINT ["dotnet", "Google.Cloud.Diagnostics.Debug.TestApp.dll"]
ENTRYPOINT ["dotnet", "/usr/share/dotnet-debugger/Google.Cloud.Diagnostics.Debug.dll", "--application-path=/app/Google.Cloud.Diagnostics.Debug.TestApp.dll", "--debugger=/usr/share/dotnet-debugger/google_cloud_debugger", "--module=standalone", "--version=3.0", "--project-id=csharp-debugger-eap"]