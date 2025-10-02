//
//  ContentView.swift
//  GrokUIDemo
//
//  Created by Grok Coding Agent
//

import SwiftUI

struct ContentView: View {
    @State private var leftPanelVisible = false
    @State private var rightPanelVisible = false
    @State private var inputText = "Type here (bottom pane)..."

    var body: some View {
        GeometryReader { geometry in
            HSplitView {
                // Left Panel
                if leftPanelVisible {
                    VStack {
                        Text("Left Tool Panel")
                            .font(.title)
                            .padding()
                        Spacer()
                        // Mock tools
                        Button("Tool 1") { print("Tool 1 clicked") }
                        Button("Tool 2") { print("Tool 2 clicked") }
                        Spacer()
                    }
                    .frame(minWidth: 200, maxWidth: 300)
                    .background(Color.gray.opacity(0.1))
                }

                // Center Content
                VSplitView {
                    // Top Pane: Read-only Rich Text
                    ScrollView {
                        Text("Main content here—read-only.\n\n**Bold text** for demo.\n\n- List item 1\n- List item 2")
                            .font(.body)
                            .padding()
                    }
                    .frame(minHeight: 300)

                    // Bottom Pane: Editable Rich Text
                    TextEditor(text: $inputText)
                        .font(.body)
                        .padding()
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                        .onChange(of: inputText) { newValue in
                            // Mock feedback
                            print("Input: \(newValue)")
                        }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)

                // Right Panel
                if rightPanelVisible {
                    VStack {
                        Text("Right Tool Panel")
                            .font(.title)
                            .padding()
                        Spacer()
                        // Mock tools
                        Button("Tool A") { print("Tool A clicked") }
                        Button("Tool B") { print("Tool B clicked") }
                        Spacer()
                    }
                    .frame(minWidth: 200, maxWidth: 300)
                    .background(Color.gray.opacity(0.1))
                }
            }
            .frame(minWidth: 800, minHeight: 600)
            .toolbar {
                // Top toolbar
                ToolbarItemGroup(placement: .automatic) {
                    Button(action: { print("New clicked") }) {
                        Label("New", systemImage: "plus")
                    }
                    Button(action: { print("Open clicked") }) {
                        Label("Open", systemImage: "folder")
                    }
                    Button(action: { print("Save clicked") }) {
                        Label("Save", systemImage: "square.and.arrow.down")
                    }
                }
            }
            .overlay(
                // Left Vertical Toolbar
                VStack(spacing: 10) {
                    Button(action: { leftPanelVisible.toggle() }) {
                        Image(systemName: leftPanelVisible ? "sidebar.left" : "sidebar.left")
                            .resizable()
                            .frame(width: 32, height: 32)
                    }
                    // More icons...
                }
                .padding(.leading, 10)
                .frame(maxHeight: .infinity, alignment: .center)
                .position(x: 40, y: geometry.size.height / 2)
                ,
                alignment: .leading
            )
            .overlay(
                // Right Vertical Toolbar
                VStack(spacing: 10) {
                    Button(action: { rightPanelVisible.toggle() }) {
                        Image(systemName: rightPanelVisible ? "sidebar.right" : "sidebar.right")
                            .resizable()
                            .frame(width: 32, height: 32)
                    }
                    // More icons...
                }
                .padding(.trailing, 10)
                .frame(maxHeight: .infinity, alignment: .center)
                .position(x: geometry.size.width - 40, y: geometry.size.height / 2)
                ,
                alignment: .trailing
            )
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}