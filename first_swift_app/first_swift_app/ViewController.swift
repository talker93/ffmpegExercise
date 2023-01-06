//
//  ViewController.swift
//  first_swift_app
//
//  Created by 江山 on 12/22/22.
//

import Cocoa

class ViewController: NSViewController {

    var recStatus: Bool = false;
    let btn = NSButton.init(title: "button", target: nil, action: nil)
    var thread: Thread?
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
        self.view.setFrameSize(NSSize(width: 320, height: 240))
        btn.title = "start recording"
        btn.frame = NSRect(x: 320/2-60, y: 240/2-30, width: 120, height: 60)
        btn.bezelStyle = .rounded
        btn.setButtonType(.pushOnPushOff)
        // callback
        btn.target = self
        btn.action = #selector(myFunc)
        self.view.addSubview(btn)
    }
    
    @objc func myFunc() {
        self.recStatus = !self.recStatus;
        
        if recStatus {
            // status: true
            // init thread
            thread = Thread.init(target: self, selector: #selector(self.recAudio), object: nil)
            thread?.start()
            self.btn.title = "stop"
        } else {
            // status: false
            // set status to 0, let the sub process finish
            set_status(0)
            self.btn.title = "start recording"
        }
    }
    
    @objc func recAudio() {
        rec_audio()
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

