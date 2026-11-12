import { motion, useMotionValue, useSpring, useTransform } from 'framer-motion'
import { ChevronRight, FileText, FolderOpen, Trash2 } from 'lucide-react'
import { useState } from 'react'

export type FolderDocument = {
  id: string
  name: string
  meta: string
  accent: string
}

type GlassFolderProps = {
  name: string
  eyebrow: string
  count: number
  tone: 'blue' | 'amber' | 'violet'
  documents: FolderDocument[]
  activeDocument?: string
  onOpenDocument: (document: FolderDocument) => void
  onDeleteDocument: (document: FolderDocument) => void
}

const toneMap = {
  blue: { glow: 'rgba(102, 198, 255, .24)', line: '#76d3ff', paper: '#204e6c' },
  amber: { glow: 'rgba(245, 181, 79, .22)', line: '#f5b54f', paper: '#694927' },
  violet: { glow: 'rgba(171, 153, 255, .22)', line: '#b3a2ff', paper: '#403866' },
}

export function GlassFolder({ name, eyebrow, count, tone, documents, activeDocument, onOpenDocument, onDeleteDocument }: GlassFolderProps) {
  const [open, setOpen] = useState(false)
  const [contextDocument, setContextDocument] = useState<string | null>(null)
  const rotateX = useMotionValue(0)
  const rotateY = useMotionValue(0)
  const springX = useSpring(rotateX, { stiffness: 220, damping: 22 })
  const springY = useSpring(rotateY, { stiffness: 220, damping: 22 })
  const lift = useTransform(springY, [-10, 10], [4, -4])
  const colors = toneMap[tone]

  const onPointerMove = (event: React.PointerEvent<HTMLButtonElement>) => {
    const rect = event.currentTarget.getBoundingClientRect()
    rotateY.set(((event.clientX - rect.left) / rect.width - 0.5) * 12)
    rotateX.set(-((event.clientY - rect.top) / rect.height - 0.5) * 12)
  }

  const resetTilt = () => {
    rotateX.set(0)
    rotateY.set(0)
  }

  return (
    <motion.section layout className={`glass-folder ${open ? 'is-open' : ''}`} style={{ '--folder-glow': colors.glow, '--folder-line': colors.line, '--folder-paper': colors.paper } as React.CSSProperties}>
      <motion.button
        className="folder-cover"
        aria-expanded={open}
        onClick={() => setOpen((value) => !value)}
        onPointerMove={onPointerMove}
        onPointerLeave={resetTilt}
        style={{ rotateX: springX, rotateY: springY, translateY: lift, transformPerspective: 800 }}
        whileTap={{ scale: 0.98 }}
      >
        <span className="folder-back" />
        <span className="folder-tab" />
        <span className="folder-glass" />
        <span className="folder-content">
          <span className="folder-topline"><span>{eyebrow}</span><span>{count.toString().padStart(2, '0')}</span></span>
          <span className="folder-name">{name}</span>
          <span className="folder-meta"><FolderOpen size={13} /> {open ? '收起文件夹' : '展开文件夹'} <ChevronRight size={13} className={open ? 'turn' : ''} /></span>
        </span>
      </motion.button>
      <motion.div layout className="folder-documents" initial={false} animate={{ height: open ? 'auto' : 0, opacity: open ? 1 : 0 }}>
        <div className="folder-document-list">
          {documents.map((document, index) => (
            <motion.div
              key={document.id}
              className={`folder-document ${activeDocument === document.id ? 'selected' : ''}`}
              role="button"
              tabIndex={0}
              onClick={() => { setContextDocument(null); onOpenDocument(document) }}
              onKeyDown={(event) => { if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); setContextDocument(null); onOpenDocument(document) } }}
              onContextMenu={(event) => { event.preventDefault(); setContextDocument(document.id) }}
              initial={{ opacity: 0, x: -10 }}
              animate={open ? { opacity: 1, x: 0 } : { opacity: 0, x: -10 }}
              transition={{ delay: open ? index * 0.06 : 0, type: 'spring', stiffness: 300, damping: 25 }}
            >
              <span className="document-icon"><FileText size={13} /></span>
              <span className="document-copy"><strong>{document.name}</strong><small>{document.meta}</small></span>
              <span className="document-dot" style={{ background: document.accent }} />
              <button className="document-delete" aria-label={`删除资料 ${document.name}`} title="删除资料" onClick={(event) => { event.stopPropagation(); onDeleteDocument(document) }}><Trash2 size={13} /></button>
              {contextDocument === document.id && <motion.div className="document-context-menu" role="menu" initial={{ opacity: 0, y: -4 }} animate={{ opacity: 1, y: 0 }} onClick={(event) => event.stopPropagation()}><button role="menuitem" onClick={() => { setContextDocument(null); onDeleteDocument(document) }}><Trash2 size={13} /> 删除资料</button></motion.div>}
            </motion.div>
          ))}
        </div>
      </motion.div>
    </motion.section>
  )
}
