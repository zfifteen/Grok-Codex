// swift-tools-version: 5.7
import PackageDescription

let package = Package(
    name: "GrokUIDemo",
    platforms: [
        .macOS(.v13)
    ],
    dependencies: [],
    targets: [
        .executableTarget(
            name: "GrokUIDemo",
            dependencies: []
        )
    ]
)