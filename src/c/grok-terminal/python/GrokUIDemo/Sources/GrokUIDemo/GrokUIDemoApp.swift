//
//  GrokUIDemoApp.swift
//  GrokUIDemo
//
//  Created by Grok Coding Agent
//

import SwiftUI

@main
struct GrokUIDemoApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
        .windowToolbarStyle(.unified(showsTitle: true))
        .defaultSize(width: 1000, height: 700)
    }
}