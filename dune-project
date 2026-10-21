(lang dune 3.18)
(using melange 0.1)

(name tea)
(version 0.1.0)

(generate_opam_files true)

(license MIT)

(authors "Syamil Abdillah <abdillahsyamil77@gmail.com>")

(source
 (github syamilAbdillah/tea-melange))

(package
 (name tea)
 (synopsis "The Elm Architecture for Melange, written in Reason")
 (description
  "One model, one update, one view, rendered through a virtual DOM. Pure core testable natively; browser layer built on melange.dom.")
 (depends
  (ocaml
   (>= 5.4))
  (dune
   (>= 3.18))
  (melange
   (>= 6.0.1))
  (reason
   (>= 3.17))
  (odoc :with-doc)))
