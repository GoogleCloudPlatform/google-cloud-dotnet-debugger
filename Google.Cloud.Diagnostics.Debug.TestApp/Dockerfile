FROM gcr.io/csharp-debugger-eap/aspnetcore:1.1.4
COPY . /app
WORKDIR /app
COPY ./source-context.json /usr/share/dotnet-debugger/agent/
ENTRYPOINT ["/usr/share/dotnet-debugger/agent/Google.Cloud.Diagnostics.Debug", "--application-start-cmd=/app/Google.Cloud.Diagnostics.Debug.TestApp.dll", "--debugger=/usr/share/dotnet-debugger/debugger/google_cloud_debugger", "--module=standalone", "--version=3.0", "--project-id=csharp-debugger-eap"]