FROM gcr.io/csharp-debugger-eap/aspnetcore:1.1.4-2017-11-08_10_44
COPY . /app
WORKDIR /app
EXPOSE 8080
ENV ASPNETCORE_URLS=http://*:8080
ENTRYPOINT ["/usr/share/dotnet-debugger/agent/Google.Cloud.Diagnostics.Debug", "--application-path=/app/Google.Cloud.Diagnostics.Debug.TestApp.dll", "--debugger=/usr/share/dotnet-debugger/debugger/google_cloud_debugger", "--module=standalone", "--version=3.0", "--project-id=csharp-debugger-eap"]